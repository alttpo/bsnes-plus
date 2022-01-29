
#include "drawlist.hpp"

namespace DrawList {

inline bool is_color_visible(uint16_t c) { return c < 0x8000; }

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

BaseTarget::BaseTarget(
  unsigned p_width,
  unsigned p_height)
  : width(p_width),
    height(p_height)
{}

inline bool BaseTarget::in_bounds(int x, int y) {
  if (y < 0) return false;
  if (y >= height) return false;
  if (x < 0) return false;
  if (x >= width) return false;
  return true;
}

void BaseTarget::draw_outlined_stroked(uint16_t stroke_color, uint16_t outline_color, const std::function<void(const plot& plot)>& draw) {
  if (!is_color_visible(stroke_color) && !is_color_visible(outline_color)) {
    return;
  }

  // record all stroked points from the shape:
  std::set<int> stroked;
  draw([this, &stroked](int x, int y) {
    if (!in_bounds(x, y))
      return;

    stroked.emplace(y*1024+x);
  });

  // now outline all stroked points without overdraw:
  for (auto it = stroked.begin(); it != stroked.end(); it++) {
    int y = *it / 1024;
    int x = *it & 1023;
    if (is_color_visible(stroke_color)) {
      px(x, y, stroke_color);
    }

    if (is_color_visible(outline_color)) {
      if (in_bounds(x-1, y-1) && stroked.find((y-1)*1024+(x-1)) == stroked.end()) {
        px(x-1, y-1, outline_color);
      }
      if (in_bounds(x+0, y-1) && stroked.find((y-1)*1024+(x+0)) == stroked.end()) {
        px(x+0, y-1, outline_color);
      }
      if (in_bounds(x+1, y-1) && stroked.find((y-1)*1024+(x+1)) == stroked.end()) {
        px(x+1, y-1, outline_color);
      }

      if (in_bounds(x-1, y+0) && stroked.find((y+0)*1024+(x-1)) == stroked.end()) {
        px(x-1, y+0, outline_color);
      }
      if (in_bounds(x+1, y+0) && stroked.find((y+0)*1024+(x+1)) == stroked.end()) {
        px(x+1, y+0, outline_color);
      }

      if (in_bounds(x-1, y+1) && stroked.find((y+1)*1024+(x-1)) == stroked.end()) {
        px(x-1, y+1, outline_color);
      }
      if (in_bounds(x+0, y+1) && stroked.find((y+1)*1024+(x+0)) == stroked.end()) {
        px(x+0, y+1, outline_color);
      }
      if (in_bounds(x+1, y+1) && stroked.find((y+1)*1024+(x+1)) == stroked.end()) {
        px(x+1, y+1, outline_color);
      }
    }
  }
};

inline void BaseTarget::draw_pixel(int x0, int y0, uint16_t color) {
  if (!in_bounds(x0, y0)) return;

  px(x0, y0, color);
}

inline void BaseTarget::draw_rect_fill(int16_t x0, int16_t y0, int16_t w, int16_t h, uint16_t fill_color) {
  if (!is_color_visible(fill_color))
    return;

  for (int16_t y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    for (int16_t x = x0; x < x0+w; x++) {
      if (x < 0) continue;
      if (x >= width) break;

      px(x, y, fill_color);
    }
  }
}

inline void BaseTarget::draw_hline(int x0, int y0, int w, const plot& px) {
  if (y0 < 0) return;
  if (y0 >= height) return;

  for (int x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= width) return;

    px(x, y0);
  }
}

inline void BaseTarget::draw_vline(int x0, int y0, int h, const plot& px) {
  if (x0 < 0) return;
  if (x0 >= width) return;

  for (int y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    px(x0, y);
  }
}

inline void BaseTarget::draw_line(int x1, int y1, int x2, int y2, const plot& px) {
  int x, y, dx, dy, dx1, dy1, mx, my, xe, ye, i;

  dx = x2 - x1;
  dy = y2 - y1;
  dx1 = fabs(dx);
  dy1 = fabs(dy);
  mx = 2 * dy1 - dx1;
  my = 2 * dx1 - dy1;

  if (dy1 <= dx1) {
    if (dx >= 0) {
      x = x1;
      y = y1;
      xe = x2;
    } else {
      x = x2;
      y = y2;
      xe = x1;
    }

    if (in_bounds(x, y)) {
      px(x, y);
    }

    for (i = 0; x < xe; i++) {
      x = x + 1;
      if (mx < 0) {
        mx = mx + 2 * dy1;
      } else {
        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
          y = y + 1;
        } else {
          y = y - 1;
        }
        mx = mx + 2 * (dy1 - dx1);
      }

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= width) break;
      if (y >= height) break;
      px(x, y);
    }
  } else {
    if (dy >= 0) {
      x = x1;
      y = y1;
      ye = y2;
    } else {
      x = x2;
      y = y2;
      ye = y1;
    }

    if (in_bounds(x, y)) {
      px(x, y);
    }

    for (i = 0; y < ye; i++) {
      y = y + 1;
      if (my <= 0) {
        my = my + 2 * dx1;
      } else {
        if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) {
          x = x + 1;
        } else {
          x = x - 1;
        }
        my = my + 2 * (dx1 - dy1);
      }

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= width) break;
      if (y >= height) break;
      px(x, y);
    }
  }
}

template<unsigned bpp>
void BaseTarget::draw_vram_tile(
  int16_t x0, int16_t y0,
  bool hflip, bool vflip,

  uint16_t vram_addr,
  uint8_t  palette,

  uint16_t twidth,
  uint16_t theight,

  uint8_t* vram,
  uint8_t* cgram
) {
  // draw tile:
  unsigned sy = y0;
  for (unsigned ty = 0; ty < theight; ty++, sy++) {
    sy &= 255;

    unsigned sx = x0;
    unsigned y = (vflip == false) ? (ty) : (theight - 1 - ty);

    for(unsigned tx = 0; tx < twidth; tx++, sx++) {
      sx &= 511;
      if(sx >= 256) continue;

      unsigned x = ((hflip == false) ? tx : (twidth - 1 - tx));

      uint8_t col, d0, d1, d2, d3, d4, d5, d6, d7;
      uint8_t mask = 1 << (7-(x&7));
      uint8_t *tile_ptr = vram + vram_addr;

      switch (bpp) {
        case 2:
          // 16 bytes per 8x8 tile
          tile_ptr += ((x >> 3) << 4);
          tile_ptr += ((y >> 3) << 8);
          tile_ptr += (y & 7) << 1;
          d0 = *(tile_ptr    );
          d1 = *(tile_ptr + 1);
          col  = !!(d0 & mask) << 0;
          col += !!(d1 & mask) << 1;
          break;
        case 4:
          // 32 bytes per 8x8 tile
          tile_ptr += ((x >> 3) << 5);
          tile_ptr += ((y >> 3) << 9);
          tile_ptr += (y & 7) << 1;
          d0 = *(tile_ptr     );
          d1 = *(tile_ptr +  1);
          d2 = *(tile_ptr + 16);
          d3 = *(tile_ptr + 17);
          col  = !!(d0 & mask) << 0;
          col += !!(d1 & mask) << 1;
          col += !!(d2 & mask) << 2;
          col += !!(d3 & mask) << 3;
          break;
        case 8:
          // 64 bytes per 8x8 tile
          tile_ptr += ((x >> 3) << 6);
          tile_ptr += ((y >> 3) << 10);
          tile_ptr += (y & 7) << 1;
          d0 = *(tile_ptr     );
          d1 = *(tile_ptr +  1);
          d2 = *(tile_ptr + 16);
          d3 = *(tile_ptr + 17);
          d4 = *(tile_ptr + 32);
          d5 = *(tile_ptr + 33);
          d6 = *(tile_ptr + 48);
          d7 = *(tile_ptr + 49);
          col  = !!(d0 & mask) << 0;
          col += !!(d1 & mask) << 1;
          col += !!(d2 & mask) << 2;
          col += !!(d3 & mask) << 3;
          col += !!(d4 & mask) << 4;
          col += !!(d5 & mask) << 5;
          col += !!(d6 & mask) << 6;
          col += !!(d7 & mask) << 7;
          break;
        default:
          // TODO: warn
          break;
      }

      // color 0 is always transparent:
      if (col == 0)
        continue;

      col += palette;

      // look up color in cgram:
      uint16_t bgr = *(cgram + (col<<1)) + (*(cgram + (col<<1) + 1) << 8);

      px(sx, sy, bgr);
    }
  }
}

uint16_t* BaseTarget::draw_image(int16_t x0, int16_t y0, int16_t w, int16_t h, uint16_t* d) {
  for (int16_t y = y0; y < y0+h; y++) {
    if (y < 0) continue;
    if (y >= height) break;

    for (int16_t x = x0; x < x0+w; x++) {
      if (x < 0) continue;
      if (x >= width) break;

      uint16_t c = *d++;
      if (!is_color_visible(c)) continue;

      px(x, y, c);
    }
  }

  return d;
}

Target::Target(
  unsigned p_width,
  unsigned p_height,
  const std::function<void(int x, int y, uint16_t color)>& p_px
) : BaseTarget(p_width, p_height), m_px(p_px)
{}

void Target::px(int x, int y, uint16_t color) {
  m_px(x, y, color);
}

Context::Context(
  const ChangeTarget& changeTarget,
  const std::shared_ptr<FontContainer>& fonts,
  const std::shared_ptr<SpaceContainer>& spaces
)
  : m_changeTarget(changeTarget), m_fonts(fonts), m_spaces(spaces)
{
  // default to OAM layer target:
  m_changeTarget(OAM, false, 15, m_target);
}

void Context::draw_list(const std::vector<uint8_t>& cmdlist) {
  uint16_t* start = (uint16_t*) cmdlist.data();
  uint32_t  end = cmdlist.size()>>1;
  uint16_t* p = start;

  uint16_t fontindex = 0;
  uint16_t colorstate[COLOR_MAX] = { 0x7fff, };
  uint16_t& stroke_color  = colorstate[COLOR_STROKE];
  uint16_t& fill_color    = colorstate[COLOR_FILL];
  uint16_t& outline_color = colorstate[COLOR_OUTLINE];

  stroke_color = 0x7fff;
  fill_color = color_none;
  outline_color = color_none;

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

        m_changeTarget(layer, pre_mode7_transform, priority, m_target);
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

          // draw tile:
          switch (bpp) {
            case 2: m_target->draw_vram_tile<2>(x0, y0, hflip, vflip, vram_addr, palette, twidth, theight, vram, cgram); break;
            case 4: m_target->draw_vram_tile<4>(x0, y0, hflip, vflip, vram_addr, palette, twidth, theight, vram, cgram); break;
            case 8: m_target->draw_vram_tile<8>(x0, y0, hflip, vflip, vram_addr, palette, twidth, theight, vram, cgram); break;
            default:
              fprintf(stderr, "draw_list: CMD_VRAM_TILE: bad bpp value %d; must be 2, 4, or 8\n", bpp);
              break;
          }
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

          d = m_target->draw_image(x0, y0, w, h, d);
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

          m_target->draw_outlined_stroked(stroke_color, outline_color, [=](const std::function<void(int,int)>& px) {
            font->draw_text_utf8(str, textchars, x0, y0, px);
          });
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

          m_target->draw_outlined_stroked(stroke_color, outline_color, [=](const std::function<void(int,int)>& px) {
            px(x0, y0);
          });
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

          m_target->draw_outlined_stroked(stroke_color, outline_color, [=](const plot& px) {
            m_target->draw_hline(x0, y0, w, px);
          });
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

          m_target->draw_outlined_stroked(stroke_color, outline_color, [=](const plot& px) {
            m_target->draw_vline(x0, y0, h, px);
          });
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

          m_target->draw_outlined_stroked(stroke_color, outline_color, [=](const plot& px) {
            m_target->draw_line(x0, y0, x1, y1, px);
          });
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

          m_target->draw_outlined_stroked(stroke_color, outline_color, [=](const plot& px) {
            m_target->draw_hline(x0,     y0,     w,   px);
            m_target->draw_hline(x0,     y0+h-1, w,   px);
            m_target->draw_vline(x0,     y0+1,   h-2, px);
            m_target->draw_vline(x0+w-1, y0+1,   h-2, px);
          });
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

          m_target->draw_rect_fill(x0, y0, w, h, fill_color);
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
