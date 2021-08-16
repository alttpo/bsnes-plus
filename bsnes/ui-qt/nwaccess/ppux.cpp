
QByteArray NWAccess::cmdPpuxReset(QByteArray args)
{
  SNES::ppu.ppux_reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdPpuxVramReset(QByteArray args)
{
  SNES::ppu.ppux_vram_reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdPpuxCgramReset(QByteArray args)
{
  SNES::ppu.ppux_cgram_reset();
  return makeOkReply();
}

void print_binary(uint32_t n) {
  for (int c = 31; c >= 0; c--) {
    uint32_t k = n >> c;

    if (k & 1)
      printf("1");
    else
      printf("0");
  }
}

QByteArray NWAccess::cmdPpuxFontUpload(QByteArray args, QByteArray data)
{
  QStringList sargs = QString::fromUtf8(args).split(';');

  QString arg = sargs.takeFirst();
  auto fontindex = toInt(arg);

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

  auto readMetrics = [&glyphs](QByteArray section) {
    QDataStream in(&section, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(QDataStream::BigEndian);
    }

    if (format & PCF_COMPRESSED_METRICS) {
      quint16 count;
      in >> count;

      glyphs.resize(count);
      for (quint16 i = 0; i < count; i++) {
        quint8 tmp;
        qint16 left_sided_bearing;
        qint16 right_side_bearing;
        qint16 character_width;
        qint16 character_ascent;
        qint16 character_descent;

        in >> tmp;
        left_sided_bearing = (qint16)tmp - 0x80;

        in >> tmp;
        right_side_bearing = (qint16)tmp - 0x80;

        in >> tmp;
        character_width = (qint16)tmp - 0x80;

        in >> tmp;
        character_ascent = (qint16)tmp - 0x80;

        in >> tmp;
        character_descent = (qint16)tmp - 0x80;

        glyphs[i].m_width = character_width;
      }
    } else {
      quint16 count;
      in >> count;

      glyphs.resize(count);
      for (quint16 i = 0; i < count; i++) {
        qint16 left_sided_bearing;
        qint16 right_side_bearing;
        qint16 character_width;
        qint16 character_ascent;
        qint16 character_descent;
        quint16 character_attributes;

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

  auto readBitmaps = [&glyphs, &bitmapdata, &fontHeight, &kmax](QByteArray section, QByteArray file) {
    QDataStream in(&section, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(QDataStream::BigEndian);
    }

    auto elemSize = (format >> 4) & 3;
    auto elemBytes = 1 << elemSize;
    kmax = (elemBytes << 3) - 1;

    quint32 count;
    in >> count;

    std::vector<quint32> offsets;
    offsets.resize(count);
    for (quint32 i = 0; i < count; i++) {
      quint32 offset;
      in >> offset;

      offsets[i] = offset;
    }

    quint32 bitmapSizes[4];
    for (quint32 i = 0; i < 4; i++) {
      in >> bitmapSizes[i];
    }

    int fontStride = 1 << (format & 3);
    //printf("bitmap stride=%d\n", fontStride);

    quint32 bitmapsSize = bitmapSizes[format & 3];
    fontHeight = (bitmapsSize / count) / fontStride;

    //printf("bitmaps size=%d, height=%d\n", bitmapsSize, fontHeight);

    QByteArray bitmapData;
    bitmapData.resize(bitmapsSize);
    in.readRawData(bitmapData.data(), bitmapsSize);

    // read bitmap data:
    glyphs.resize(count);
    for (quint32 i = 0; i < count; i++) {
      quint32 size = 0;
      if (i < count-1) {
        size = offsets[i+1] - offsets[i];
      } else {
        size = bitmapsSize - offsets[i];
      }

      // find where to read from:
      QDataStream bits(&bitmapData, QIODevice::ReadOnly);
      bits.skipRawData(offsets[i]);

      glyphs[i].m_bitmapdata.resize(size / fontStride);

      //printf("[%3d]\n", i);
      int y = 0;
      for (int k = 0; k < size; k += fontStride, y++) {
        quint32 b;
        if (elemSize == 0) {
          quint8  w;
          bits >> w;
          b = w;
        } else if (elemSize == 1) {
          quint16 w;
          bits >> w;
          b = w;
        } else if (elemSize == 2) {
          quint32 w;
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

  auto readEncodings = [&index](QByteArray section) {
    QDataStream in(&section, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(QDataStream::BigEndian);
    }

    quint16 min_char_or_byte2;
    quint16 max_char_or_byte2;
    quint16 min_byte1;
    quint16 max_byte1;
    quint16 default_char;

    in >> min_char_or_byte2;
    in >> max_char_or_byte2;
    in >> min_byte1;
    in >> max_byte1;
    in >> default_char;

    //printf("[%02x..%02x], [%02x..%02x]\n", min_byte1, max_byte1, min_char_or_byte2, max_char_or_byte2);

    quint32 byte2count = (max_char_or_byte2-min_char_or_byte2+1);
    quint32 count = byte2count * (max_byte1-min_byte1+1);
    quint16 glyphindices[count];
    for (quint32 i = 0; i < count; i++) {
      in >> glyphindices[i];
      //printf("%4d\n", glyphindices[i]);
    }

    // construct an ordered list of index ranges:
    quint32 startIndex = 0xFFFF;
    quint16 startCodePoint = 0xFFFF;
    quint16 endCodePoint = 0xFFFF;
    quint32 i = 0;
    for (quint32 b1 = min_byte1; b1 <= max_byte1; b1++) {
      for (quint32 b2 = min_char_or_byte2; b2 <= max_char_or_byte2; b2++, i++) {
        quint16 x = glyphindices[i];

        // calculate code point:
        quint32 cp = (b1<<8) + b2;

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

  auto readPCF = [&]() -> QByteArray {
    // parse PCF data:
    QDataStream in(&data, QIODevice::ReadOnly);

    char hdr[4];
    in.readRawData(hdr, 4);
    if (strncmp(hdr, "\1fcp", 4) != 0) {
      return makeErrorReply("expected PCF file format header not found");
    }

    // read little endian aka LSB first:
    in.setByteOrder(QDataStream::LittleEndian);
    qint32 table_count;
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

    for (qint32 t = 0; t < table_count; t++) {
      qint32 type, table_format, size, offset;
      in >> type >> table_format >> size >> offset;

      switch (type) {
        case PCF_METRICS:
          readMetrics(data.mid(offset, size));
          break;
        case PCF_BITMAPS:
          readBitmaps(data.mid(offset, size), data);
          break;
        case PCF_BDF_ENCODINGS:
          readEncodings(data.mid(offset, size));
          break;
      }
    }

    // add in new font at requested index:
    wasmInterface.fonts.resize(fontindex+1);
    wasmInterface.fonts[fontindex].reset(
      new PixelFont::Font(glyphs, index, fontHeight, kmax)
    );

    return makeOkReply();
  };

  return readPCF();
}
