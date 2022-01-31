
#include "drawlist.hpp"

namespace DrawList {

LocalSpace::LocalSpace() {}

uint8_t* LocalSpace::vram_data() { return SNES::memory::vram.data(); }
const uint32_t LocalSpace::vram_size() const { return 0x10000; }

uint8_t* LocalSpace::cgram_data() { return SNES::memory::cgram.data(); }
const uint32_t LocalSpace::cgram_size() const { return 0x200; }

ExtraSpace::ExtraSpace() {}

uint8_t* ExtraSpace::vram_data() { return vram; }
const uint32_t ExtraSpace::vram_size() const { return 0x10000; }

uint8_t* ExtraSpace::cgram_data() { return cgram; }
const uint32_t ExtraSpace::cgram_size() const { return 0x200; }

SpaceContainer::SpaceContainer() {
  m_localSpace.reset(new LocalSpace());
  m_spaces.resize(MaxCount);
}

void SpaceContainer::reset() {
  m_spaces.clear();
}

std::shared_ptr<Space> SpaceContainer::operator[](int index) {
  if (index > MaxCount) {
    throw std::out_of_range("index out of range");
  }

  if (index == 0) {
    return m_localSpace;
  }

  auto& space = m_spaces[index-1];
  if (!space) {
    space.reset(new ExtraSpace());
  }

  return space;
}

uint8_t* SpaceContainer::get_vram_space(int index) {
  const std::shared_ptr<Space> &space = operator[](index);
  if (!space)
    return nullptr;

  return space->vram_data();
}
uint8_t* SpaceContainer::get_cgram_space(int index) {
  const std::shared_ptr<Space> &space = operator[](index);
  if (!space)
    return nullptr;

  return space->cgram_data();
}

Context::Context(
  const ChooseRenderer& chooseRenderer,
  const std::shared_ptr<FontContainer>& fonts,
  const std::shared_ptr<SpaceContainer>& spaces
)
  : m_chooseRenderer(chooseRenderer), m_fonts(fonts), m_spaces(spaces)
{
  // default to OAM layer target:
  m_chooseRenderer(OAM, false, 15, m_renderer);
}

void Context::draw_list(const std::vector<uint16_t>& cmdlist) {
  uint16_t* start = (uint16_t*) cmdlist.data();
  uint32_t  end = cmdlist.size();
  uint16_t* p = start;

  uint16_t fontindex = 0;
  uint16_t colorstate[COLOR_MAX] = { 0x7fff, };
  uint16_t& stroke_color  = colorstate[COLOR_STROKE];
  uint16_t& fill_color    = colorstate[COLOR_FILL];
  uint16_t& outline_color = colorstate[COLOR_OUTLINE];

  stroke_color = 0x7fff;
  fill_color = color_none;
  outline_color = color_none;

  m_renderer->set_stroke_color(colorstate[COLOR_STROKE]);
  m_renderer->set_fill_color(colorstate[COLOR_FILL]);
  m_renderer->set_outline_color(colorstate[COLOR_OUTLINE]);

  // process all commands:
  while ((p - start) < end) {
    // every command starts with the number of 16-bit words in length, including command, arguments, and inline data:
    uint16_t len = *p++;

    if (p + len - start > end) {
      fprintf(stderr, "draw_list: command length at index %ld exceeds size of command list; %lu > %u", p - start, p + len - start, end);
      break;
    }

    uint16_t* d = p;
    p += len;

    uint16_t cmd = *d++;

    // set start to start of arguments of command and adjust len to concern only arguments:
    uint16_t* args = d;
    len--;

    int16_t  x0, y0, w, h;
    int16_t  x1, y1;
    switch (cmd) {
      case CMD_TARGET: {
        draw_layer layer = (draw_layer) *d++;
        bool pre_mode7_transform = *d++;
        uint8_t priority = *d++;

        m_chooseRenderer(layer, pre_mode7_transform, priority, m_renderer);
        break;
      }
      case CMD_VRAM_TILE: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 11) {
            fprintf(stderr, "draw_list: CMD_VRAM_TILE: incomplete command; %ld < %d\n", len-(d-args), 11);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          bool   hflip = (bool)*d++;
          bool   vflip = (bool)*d++;

          uint16_t vram_space = *d++;       //     0 ..  1023; 0 = local, 1..1023 = extra
          uint16_t vram_addr = *d++;        // $0000 .. $FFFF (byte address)
          uint16_t cgram_space = *d++;      //     0 ..  1023; 0 = local, 1..1023 = extra
          uint8_t  palette = *d++;          //     0 ..   255

          uint8_t  bpp = *d++;              // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
          uint16_t twidth = *d++;           // number of pixels width
          uint16_t theight = *d++;          // number of pixels high

          uint8_t* vram = m_spaces->get_vram_space(vram_space);
          if (!vram) {
            fprintf(stderr, "draw_list: CMD_VRAM_TILE: bad VRAM space; %d\n", vram_space);
            continue;
          }

          uint8_t* cgram = m_spaces->get_cgram_space(cgram_space);
          if (!cgram) {
            fprintf(stderr, "draw_list: CMD_VRAM_TILE: bad CGRAM space; %d\n", cgram_space);
            continue;
          }

          m_renderer->draw_vram_tile(x0, y0, twidth, theight, hflip, vflip, bpp, vram_addr, palette, vram, cgram);
        }
        break;
      }
      case CMD_IMAGE: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 4) {
            fprintf(stderr, "draw_list: CMD_IMAGE: incomplete command; %ld < %d\n", len-(d-args), 4);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          w  = (int16_t)*d++;
          h  = (int16_t)*d++;

          // check again after getting width and height:
          if (len - (d - args) < w*h) {
            fprintf(stderr, "draw_list: CMD_IMAGE: incomplete command; %ld < %d\n", len-(d-args), w*h);
            break;
          }

          d = m_renderer->draw_image(x0, y0, w, h, d);
        }
        break;
      }
      case CMD_COLOR_DIRECT_BGR555: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 2) {
            fprintf(stderr, "draw_list: CMD_COLOR_DIRECT_BGR555: incomplete command; %ld < %d\n", len-(d-args), 2);
            break;
          }

          uint16_t index = *d++;
          uint16_t color = *d++;

          if (index >= COLOR_MAX) {
            fprintf(stderr, "draw_list: CMD_COLOR_DIRECT_BGR555: bad COLOR_TYPE index; %hu >= %d\n", index, COLOR_MAX);
            continue;
          }

          colorstate[index] = color;

          m_renderer->set_stroke_color(colorstate[COLOR_STROKE]);
          m_renderer->set_fill_color(colorstate[COLOR_FILL]);
          m_renderer->set_outline_color(colorstate[COLOR_OUTLINE]);
        }
        break;
      }
      case CMD_COLOR_DIRECT_RGB888: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 3) {
            fprintf(stderr, "draw_list: CMD_COLOR_DIRECT_RGB888: incomplete command; %ld < %d\n", len-(d-args), 3);
            break;
          }

          uint16_t index = *d++;

          // 0bAAAAAAAARRRRRRRR
          uint16_t ar = *d++;
          // 0bGGGGGGGGBBBBBBBB
          uint16_t gb = *d++;

          if (index >= COLOR_MAX) {
            fprintf(stderr, "draw_list: CMD_COLOR_DIRECT_RGB888: bad COLOR_TYPE index; %hu >= %d\n", index, COLOR_MAX);
            continue;
          }

          // convert to BGR555:
          colorstate[index] =
            // blue
            (((gb >> 3) & 0x1F) << 10)
            // green
            | (((gb >> 11) & 0x1F) << 5)
            // red
            | ((ar >> 3) & 0x1F)
            // alpha (only MSB)
            | ((ar >> 15) << 15);

          m_renderer->set_stroke_color(colorstate[COLOR_STROKE]);
          m_renderer->set_fill_color(colorstate[COLOR_FILL]);
          m_renderer->set_outline_color(colorstate[COLOR_OUTLINE]);
        }
        break;
      }
      case CMD_COLOR_PALETTED: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 3) {
            fprintf(stderr, "draw_list: CMD_COLOR_PALETTED: incomplete command; %ld < %d\n", len-(d-args), 3);
            break;
          }

          uint16_t index = *d++;
          uint16_t space = *d++;
          unsigned addr = ((uint16_t)*d++) << 1;

          if (index >= COLOR_MAX) {
            fprintf(stderr, "draw_list: CMD_COLOR_PALETTED: bad COLOR_TYPE index; %hu >= %d\n", index, COLOR_MAX);
            continue;
          }

          uint8_t* cgram = m_spaces->get_cgram_space(space);
          if(!cgram) {
            fprintf(stderr, "draw_list: CMD_COLOR_PALETTED: bad CGRAM space; %d\n", space);
            continue;
          }

          // read litle-endian 16-bit color from CGRAM:
          colorstate[index] = cgram[addr] + (cgram[addr + 1] << 8);

          m_renderer->set_stroke_color(colorstate[COLOR_STROKE]);
          m_renderer->set_fill_color(colorstate[COLOR_FILL]);
          m_renderer->set_outline_color(colorstate[COLOR_OUTLINE]);
        }
        break;
      }
      case CMD_FONT_SELECT: {
        // select font:
        uint16_t index = *d++;
        if (index >= m_fonts->size()) {
          fprintf(stderr, "draw_list: CMD_FONT_SELECT: bad font index; %d\n", index);
          continue;
        }
        fontindex = index;
        break;
      }
      case CMD_TEXT_UTF8: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 3) {
            fprintf(stderr, "draw_list: CMD_TEXT_UTF8: incomplete command; %ld < %d\n", len-(d-args), 3);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          uint16_t textchars = *d++;

          uint8_t* str = (uint8_t*)d;

          uint16_t textwords = textchars;

          // include the last pad byte if odd sized:
          if (textwords & 1) {
            textwords++;
          }
          textwords >>= 1;

          // need a complete command:
          if (len - (d - args) < textwords) {
            fprintf(stderr, "draw_list: CMD_TEXT_UTF8: incomplete text data; %ld < %d\n", len-(d-args), textwords);
            break;
          }

          d += textwords;

          // find font:
          if (fontindex >= m_fonts->size()) {
            continue;
          }
          const auto font = (*m_fonts)[fontindex];
          if (!font) {
            continue;
          }

          m_renderer->draw_text_utf8(str, textchars, *font, x0, y0);
        }
        break;
      }
      case CMD_PIXEL: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 2) {
            fprintf(stderr, "draw_list: CMD_PIXEL: incomplete command; %ld < %d\n", len-(d-args), 2);
            break;
          }

          x0 = *d++;
          y0 = *d++;

          m_renderer->draw_pixel(x0, y0);
        }
        break;
      }
      case CMD_HLINE: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 3) {
            fprintf(stderr, "draw_list: CMD_HLINE: incomplete command; %ld < %d\n", len-(d-args), 3);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          w  = (int16_t)*d++;

          m_renderer->draw_hline(x0, y0, w);
        }
        break;
      }
      case CMD_VLINE: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 3) {
            fprintf(stderr, "draw_list: CMD_VLINE: incomplete command; %ld < %d\n", len-(d-args), 3);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          h  = (int16_t)*d++;

          m_renderer->draw_vline(x0, y0, w);
        }
        break;
      }
      case CMD_LINE: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 4) {
            fprintf(stderr, "draw_list: CMD_LINE: incomplete command; %ld < %d\n", len-(d-args), 4);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          x1 = (int16_t)*d++;
          y1 = (int16_t)*d++;

          m_renderer->draw_line(x0, y0, x1, y1);
        }
        break;
      }
      case CMD_RECT: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 4) {
            fprintf(stderr, "draw_list: CMD_RECT: incomplete command; %ld < %d\n", len-(d-args), 4);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          w  = (int16_t)*d++;
          h  = (int16_t)*d++;

          m_renderer->draw_rect(x0, y0, w, h);
        }
        break;
      }
      case CMD_RECT_FILL: {
        while (d - args < len) {
          // need a complete command:
          if (len - (d - args) < 4) {
            fprintf(stderr, "draw_list: CMD_RECT_FILL: incomplete command; %ld < %d\n", len-(d-args), 4);
            break;
          }

          x0 = (int16_t)*d++;
          y0 = (int16_t)*d++;
          w  = (int16_t)*d++;
          h  = (int16_t)*d++;

          m_renderer->draw_rect_fill(x0, y0, w, h);
        }
        break;
      }
    }
  }
}

void FontContainer::clear() {
  m_fonts.clear();
}

void FontContainer::erase(int fontindex) {
  m_fonts.erase(m_fonts.begin() + fontindex);
}

std::shared_ptr<PixelFont::Font> FontContainer::operator[](int fontindex) const {
  return m_fonts[fontindex];
}

int FontContainer::size() const {
  return m_fonts.size();
}

struct ByteArray {
  explicit ByteArray() {
    //printf("%p->ByteArray()\n", this);
    m_data = nullptr;
    m_size = 0;
  }
  explicit ByteArray(const uint8_t* data, int size) {
    //printf("%p->ByteArray(%p, %d)\n", this, data, size);
    if (size < 0) {
      throw std::invalid_argument("size cannot be negative");
    }
    m_data = new uint8_t[size];
    m_size = size;
    memcpy((void *)m_data, (const void *)data, size);
    //printf("%p->ByteArray = (%p, %d)\n", this, m_data, m_size);
  }

  ~ByteArray() {
    //printf("%p->~ByteArray() = (%p, %d)\n", this, m_data, m_size);
    delete m_data;
    m_data = nullptr;
    m_size = 0;
  }

  uint8_t* data() { return m_data; }
  const uint8_t* data() const { return m_data; }

  ByteArray mid(int offset, int size) {
    if (offset > m_size) {
      return ByteArray();
    }

    if (offset < 0) {
      if (size < 0 || offset + size >= m_size) {
        return ByteArray(m_data, m_size);
      }
      if (offset + size <= 0) {
        return ByteArray();
      }
      size += offset;
      offset = 0;
    } else if (size_t(size) > size_t(m_size - offset)) {
      size = m_size - offset;
    }

    return ByteArray(m_data + offset, size);
  }

  ByteArray& resize(int size) {
    m_size = size;
    if (m_data == nullptr) {
      m_data = new uint8_t[size];
      return *this;
    }

    // todo:
    throw std::runtime_error("unimplemented resize()");
  }

private:
  uint8_t*  m_data;
  int       m_size;
};

struct DataStream {
  enum ByteOrder {
    LittleEndian,
    BigEndian
  };

  DataStream(const ByteArray& a) : m_a(a), p(a.data()) {}

  DataStream& setByteOrder(ByteOrder o) {
    m_o = o;
    return *this;
  }

  DataStream& operator>>(uint8_t& v) {
    v = *p++;
    return *this;
  }

  DataStream& operator>>(int16_t& v) {
    // todo: bounds checking
    uint16_t t;
    switch (m_o) {
      case LittleEndian:
        t  = (uint16_t)*p++;
        t |= ((uint16_t)*p++) << 8;
        v  = (int16_t)t;
        return *this;
      case BigEndian:
        t  = ((uint16_t)*p++) << 8;
        t |= ((uint16_t)*p++);
        v  = (int16_t)t;
        return *this;
    }
    return *this;
  }

  DataStream& operator>>(uint16_t& v) {
    // todo: bounds checking
    switch (m_o) {
      case LittleEndian:
        v  = (uint16_t)*p++;
        v |= ((uint16_t)*p++) << 8;
        return *this;
      case BigEndian:
        v  = ((uint16_t)*p++) << 8;
        v |= ((uint16_t)*p++);
        return *this;
    }
    return *this;
  }

  DataStream& operator>>(int32_t& v) {
    // todo: bounds checking
    uint32_t t;
    switch (m_o) {
      case LittleEndian:
        t  = ((uint32_t)*p++);
        t |= ((uint32_t)*p++) << 8;
        t |= ((uint32_t)*p++) << 16;
        t |= ((uint32_t)*p++) << 24;
        v = (int32_t)t;
        return *this;
      case BigEndian:
        t  = ((uint32_t)*p++) << 24;
        t |= ((uint32_t)*p++) << 16;
        t |= ((uint32_t)*p++) << 8;
        t |= ((uint32_t)*p++);
        v = (int32_t)t;
        return *this;
    }
    return *this;
  }

  DataStream& operator>>(uint32_t& v) {
    // todo: bounds checking
    switch (m_o) {
      case LittleEndian:
        v  = ((uint32_t)*p++ & 0xFF);
        v |= ((uint32_t)*p++ & 0xFF) << 8;
        v |= ((uint32_t)*p++ & 0xFF) << 16;
        v |= ((uint32_t)*p++ & 0xFF) << 24;
        return *this;
      case BigEndian:
        v  = ((uint32_t)*p++ & 0xFF) << 24;
        v |= ((uint32_t)*p++ & 0xFF) << 16;
        v |= ((uint32_t)*p++ & 0xFF) << 8;
        v |= ((uint32_t)*p++ & 0xFF);
        return *this;
    }
    return *this;
  }

  int readRawData(uint8_t *s, int len) {
    // todo: bounds checking
    memcpy((void *)s, (const void *)p, len);
    p += len;
    return len;
  }

  int skipRawData(int len) {
    // todo: bounds checking
    p += len;
    return len;
  }

private:
  const ByteArray&  m_a;
  const uint8_t*    p;

  ByteOrder m_o;
};

void print_binary(uint32_t n) {
  for (int c = 31; c >= 0; c--) {
    uint32_t k = n >> c;

    if (k & 1)
      printf("1");
    else
      printf("0");
  }
}

void FontContainer::load_pcf(int fontindex, const uint8_t* pcf_data, int pcf_size) {
  ByteArray data(pcf_data, pcf_size);

#define PCF_DEFAULT_FORMAT      0x00000000
#define PCF_INKBOUNDS           0x00000200
#define PCF_ACCEL_W_INKBOUNDS   0x00000100
#define PCF_COMPRESSED_METRICS  0x00000100

#define PCF_GLYPH_PAD_MASK      (3<<0)      /* See the bitmap table for explanation */
#define PCF_BYTE_MASK           (1<<2)      /* If set then Most Sig Byte First */
#define PCF_BIT_MASK            (1<<3)      /* If set then Most Sig Bit First */
#define PCF_SCAN_UNIT_MASK      (3<<4)      /* See the bitmap table for explanation */

  std::vector<PixelFont::Glyph> glyphs;
  std::vector<PixelFont::Index> index;
  std::vector<uint8_t>          bitmapdata;
  int fontHeight;
  int kmax;

  auto readMetrics = [&glyphs](ByteArray& section) {
    DataStream in(section);
    in.setByteOrder(DataStream::LittleEndian);

    uint32_t format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(DataStream::BigEndian);
    }

    if (format & PCF_COMPRESSED_METRICS) {
      uint16_t count;
      in >> count;

      glyphs.resize(count);
      for (uint16_t i = 0; i < count; i++) {
        uint8_t tmp;
        int16_t left_sided_bearing;
        int16_t right_side_bearing;
        int16_t character_width;
        int16_t character_ascent;
        int16_t character_descent;

        in >> tmp;
        left_sided_bearing = (int16_t)tmp - 0x80;

        in >> tmp;
        right_side_bearing = (int16_t)tmp - 0x80;

        in >> tmp;
        character_width = (int16_t)tmp - 0x80;

        in >> tmp;
        character_ascent = (int16_t)tmp - 0x80;

        in >> tmp;
        character_descent = (int16_t)tmp - 0x80;

        glyphs[i].m_width = character_width;
      }
    } else {
      uint16_t count;
      in >> count;

      glyphs.resize(count);
      for (uint16_t i = 0; i < count; i++) {
        int16_t left_sided_bearing;
        int16_t right_side_bearing;
        int16_t character_width;
        int16_t character_ascent;
        int16_t character_descent;
        uint16_t character_attributes;

        in >> left_sided_bearing;
        in >> right_side_bearing;
        in >> character_width;
        in >> character_ascent;
        in >> character_descent;
        in >> character_attributes;

        glyphs[i].m_width = character_width;
      }
    }
  };

  auto readBitmaps = [&glyphs, &bitmapdata, &fontHeight, &kmax](ByteArray& section) {
    DataStream in(section);
    in.setByteOrder(DataStream::LittleEndian);

    uint32_t format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(DataStream::BigEndian);
    }

    auto elemSize = (format >> 4) & 3;
    auto elemBytes = 1 << elemSize;
    kmax = (elemBytes << 3) - 1;

    uint32_t count;
    in >> count;
    //printf("bitmap count=%u\n", count);

    std::vector<uint32_t> offsets;
    offsets.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      uint32_t offset;
      in >> offset;

      offsets[i] = offset;
    }

    uint32_t bitmapSizes[4];
    for (uint32_t i = 0; i < 4; i++) {
      in >> bitmapSizes[i];
    }

    int fontStride = 1 << (format & 3);
    //printf("bitmap stride=%d\n", fontStride);

    uint32_t bitmapsSize = bitmapSizes[format & 3];
    fontHeight = (bitmapsSize / count) / fontStride;

    //printf("bitmaps size=%d, height=%d\n", bitmapsSize, fontHeight);

    ByteArray bitmapData;
    bitmapData.resize(bitmapsSize);
    in.readRawData(bitmapData.data(), bitmapsSize);

    // read bitmap data:
    glyphs.resize(count);
    for (uint32_t i = 0; i < count; i++) {
      uint32_t size = 0;
      if (i < count-1) {
        size = offsets[i+1] - offsets[i];
      } else {
        size = bitmapsSize - offsets[i];
      }

      // find where to read from:
      DataStream bits(bitmapData);
      bits.skipRawData(offsets[i]);

      glyphs[i].m_bitmapdata.resize(size / fontStride);

      //printf("[%3d]\n", i);
      int y = 0;
      for (int k = 0; k < size; k += fontStride, y++) {
        uint32_t b;
        if (elemSize == 0) {
          uint8_t  w;
          bits >> w;
          b = w;
        } else if (elemSize == 1) {
          uint16_t w;
          bits >> w;
          b = w;
        } else if (elemSize == 2) {
          uint32_t w;
          bits >> w;
          b = w;
        }
        bits.skipRawData(fontStride - elemBytes);

        // TODO: account for bit order
        glyphs[i].m_bitmapdata[y] = b;
        //print_binary(b);
        //putc('\n', stdout);
      }
    }
  };

  auto readEncodings = [&index](ByteArray& section) {
    DataStream in(section);
    in.setByteOrder(DataStream::LittleEndian);

    uint32_t format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(DataStream::BigEndian);
    }

    uint16_t min_char_or_byte2;
    uint16_t max_char_or_byte2;
    uint16_t min_byte1;
    uint16_t max_byte1;
    uint16_t default_char;

    in >> min_char_or_byte2;
    in >> max_char_or_byte2;
    in >> min_byte1;
    in >> max_byte1;
    in >> default_char;

    //printf("[%02x..%02x], [%02x..%02x]\n", min_byte1, max_byte1, min_char_or_byte2, max_char_or_byte2);

    uint32_t byte2count = (max_char_or_byte2-min_char_or_byte2+1);
    uint32_t count = byte2count * (max_byte1-min_byte1+1);
    uint16_t glyphindices[count];
    for (uint32_t i = 0; i < count; i++) {
      in >> glyphindices[i];
      //printf("%4d\n", glyphindices[i]);
    }

    // construct an ordered list of index ranges:
    uint32_t startIndex = 0xFFFF;
    uint16_t startCodePoint = 0xFFFF;
    uint16_t endCodePoint = 0xFFFF;
    uint32_t i = 0;
    for (uint32_t b1 = min_byte1; b1 <= max_byte1; b1++) {
      for (uint32_t b2 = min_char_or_byte2; b2 <= max_char_or_byte2; b2++, i++) {
        uint16_t x = glyphindices[i];

        // calculate code point:
        uint32_t cp = (b1<<8) + b2;

        if (x == 0xFFFF) {
          if (startIndex != 0xFFFF) {
            index.emplace_back(startIndex, startCodePoint, endCodePoint);
            startIndex = 0xFFFF;
            startCodePoint = 0xFFFF;
          }
        } else {
          if (startIndex == 0xFFFF) {
            startIndex = x;
            startCodePoint = cp;
          }
          endCodePoint = cp;
        }
      }
    }

    if (startIndex != 0xFFFF) {
      index.emplace_back(startIndex, startCodePoint, endCodePoint);
      startIndex = 0xFFFF;
      startCodePoint = 0xFFFF;
    }

    //for (const auto& i : index) {
    //  printf("index[%4d @ %4d..%4d]\n", i.m_glyphIndex, i.m_minCodePoint, i.m_maxCodePoint);
    //}
  };

  auto readPCF = [&]() {
    // parse PCF data:
    DataStream in(data);

    uint8_t hdr[4];
    in.readRawData(hdr, 4);
    if (strncmp((char *)hdr, "\1fcp", 4) != 0) {
      throw std::runtime_error("expected PCF file format header not found");
    }

    // read little endian aka LSB first:
    in.setByteOrder(DataStream::LittleEndian);
    uint32_t table_count;
    in >> table_count;

#define PCF_PROPERTIES              (1<<0)
#define PCF_ACCELERATORS            (1<<1)
#define PCF_METRICS                 (1<<2)
#define PCF_BITMAPS                 (1<<3)
#define PCF_INK_METRICS             (1<<4)
#define PCF_BDF_ENCODINGS           (1<<5)
#define PCF_SWIDTHS                 (1<<6)
#define PCF_GLYPH_NAMES             (1<<7)
#define PCF_BDF_ACCELERATORS        (1<<8)

    for (int32_t t = 0; t < table_count; t++) {
      int32_t type, table_format, size, offset;
      in >> type >> table_format >> size >> offset;
      //printf("%d %d %d %d\n", type, table_format, size, offset);

      switch (type) {
        case PCF_METRICS: {
          ByteArray section = data.mid(offset, size);
          readMetrics(section);
          break;
        }
        case PCF_BITMAPS: {
          ByteArray section = data.mid(offset, size);
          readBitmaps(section);
          break;
        }
        case PCF_BDF_ENCODINGS: {
          ByteArray section = data.mid(offset, size);
          readEncodings(section);
          break;
        }
      }
    }

    // add in new font at requested index:
    m_fonts.resize(fontindex+1);
    m_fonts[fontindex].reset(
      new PixelFont::Font(glyphs, index, fontHeight, kmax)
    );
  };

  readPCF();
}

}
