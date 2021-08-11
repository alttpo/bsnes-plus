
QByteArray NWAccess::cmdPpuxReset(QByteArray args)
{
  SNES::ppu.ppux_reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdPpuxSpriteReset(QByteArray args)
{
  SNES::ppu.ppux_sprite_reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdPpuxSpriteWrite(QByteArray args)
{
  QStringList sprs = QString::fromUtf8(args).split('|');

  QString reply;
  while (!sprs.isEmpty()) {
    QStringList sargs = sprs.takeFirst().split(';');
    if (sargs.isEmpty()) continue;

    QString arg = sargs.takeFirst();
    auto index = toInt(arg);
    if (index < 0 || index >= SNES::PPU::extra_count)
      return makeErrorReply(QString("index must be 0..%1").arg(SNES::PPU::extra_count-1));

    if (!reply.isEmpty()) reply.append("\n");
    reply += QString("index:%1").arg(index);

    auto t = &SNES::ppu.extra_list[index];

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //bool   enabled;
      t->enabled = toInt(arg);
    }
    reply += QString("\nenabled:%1").arg(t->enabled);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //uint16 x;
      t->x = toInt(arg);
    }
    reply += QString("\nx:%1").arg(t->x);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //uint16 y;
      t->y = toInt(arg);
    }
    reply += QString("\ny:%1").arg(t->y);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //bool   hflip;
      t->hflip = toInt(arg);
    }
    reply += QString("\nhflip:%1").arg(t->hflip);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //bool   vflip;
      t->vflip = toInt(arg);
    }
    reply += QString("\nvflip:%1").arg(t->vflip);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      uint8_t vram_space;
      vram_space = toInt(arg);
      if (vram_space < 0 || vram_space >= SNES::PPU::extra_spaces) return makeErrorReply(QString("vram_space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
      t->vram_space = vram_space;
    }
    reply += QString("\nvram_space:%1").arg(t->vram_space);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //uint16 vram_addr;
      t->vram_addr = toInt(arg);
    }
    reply += QString("\nvram_addr:%1").arg(t->vram_addr);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      uint8_t cgram_space;
      cgram_space = toInt(arg);
      if (cgram_space < 0 || cgram_space >= SNES::PPU::extra_spaces) return makeErrorReply(QString("cgram_space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
      t->cgram_space = cgram_space;
    }
    reply += QString("\ncgram_space:%1").arg(t->cgram_space);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //uint8 palette;
      t->palette = toInt(arg);
    }
    reply += QString("\npalette:%1").arg(t->palette);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      uint8_t  layer;    // 0..4;  BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4
      layer = toInt(arg);
      t->layer = layer;
    }
    reply += QString("\nlayer:%1").arg(t->layer);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      uint8_t  priority; // 1..12
      priority = toInt(arg);
      t->priority = priority;
    }
    reply += QString("\npriority:%1").arg(t->priority);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //bool   color_exemption;
      t->color_exemption = toInt(arg);
    }
    reply += QString("\ncolor_exemption:%1").arg(t->color_exemption);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      uint8_t  bpp;
      bpp = toInt(arg);
      if (bpp != 2 && bpp != 4 && bpp != 8) return makeErrorReply("bpp can only be one of [2, 4, 8]");
      t->bpp = bpp;
    }
    reply += QString("\nbpp:%1").arg(t->bpp);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //uint16 width;
      t->width = toInt(arg);
    }
    reply += QString("\nwidth:%1").arg(t->width);

    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      //uint16 height;
      t->height = toInt(arg);
    }
    reply += QString("\nheight:%1").arg(t->height);
  }

  return makeHashReply(reply);
}

QByteArray NWAccess::cmdPpuxRamWrite(QByteArray args, QByteArray data)
{
  QStringList sargs = QString::fromUtf8(args).split(';');

  QString reply;
  while (!sargs.isEmpty()) {
    QString arg = sargs.takeFirst();
    QString memory = arg;

    if (!reply.isEmpty()) reply.append("\n");
    reply += QString("memory:%1").arg(memory);

    arg = sargs.takeFirst();
    auto space = toInt(arg);
    if (space < 0 || space >= SNES::PPU::extra_spaces) return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));

    reply += QString("\nspace:%1").arg(space);

    unsigned maxSize = 0;
    uint8_t *t = nullptr;
    if (memory == "VRAM") {
      t = SNES::ppu.get_vram_space(space);
      maxSize = 0x10000;
    } else if (memory == "CGRAM") {
      t = SNES::ppu.get_cgram_space(space);
      maxSize = 0x200;
    } else {
      return makeErrorReply(QString("unknown memory type '%1' expected 'VRAM' or 'CGRAM'").arg(memory));
    }
    if (!t) return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));

    unsigned offset = 0;
    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      offset = toInt(arg);
    }
    if (offset >= maxSize) return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    if (offset & 1) return makeErrorReply("offset must be multiple of 2");

    unsigned size = 0;
    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      size = toInt(arg);
    }
    if (size > data.size()) return makeErrorReply(QString("size $%1 > payload $%2").arg(size, 0, 16).arg(data.size(), 0, 16));

    if (offset + size > maxSize) return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    reply += QString("\noffset:%1").arg(offset);
    reply += QString("\nsize:%1").arg(size);

    uint8_t *d = (uint8_t*)data.data();
    for (unsigned i = 0; i < size; i++) {
      *(t + offset + i) = d[i];
    }
    data = data.mid(size);
  }

  return makeHashReply(reply);
}

QByteArray NWAccess::cmdPpuxRamRead(QByteArray args)
{
  QByteArray data;

  QStringList regions = QString::fromUtf8(args).split('|');
  while (!regions.isEmpty()) {
    QStringList sargs = regions.takeFirst().split(';');

    QString arg = sargs.takeFirst();
    QString memory = arg;

    arg = sargs.takeFirst();
    auto space = toInt(arg);
    if (space < 0 || space >= SNES::PPU::extra_spaces) return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));

    unsigned maxSize = 0;
    uint8_t *t = nullptr;
    if (memory == "VRAM") {
      t = SNES::ppu.get_vram_space(space);
      maxSize = 0x10000;
    } else if (memory == "CGRAM") {
      t = SNES::ppu.get_cgram_space(space);
      maxSize = 0x200;
    } else {
      return makeErrorReply(QString("unknown memory type '%1' expected 'VRAM' or 'CGRAM'").arg(memory));
    }
    if (!t) return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));

    unsigned offset = 0;
    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      offset = toInt(arg);
    }
    if (offset >= maxSize) return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    if (offset & 1) return makeErrorReply("offset must be multiple of 2");

    unsigned size = 0;
    arg = sargs.isEmpty() ? QString() : sargs.takeFirst();
    if (!arg.isEmpty()) {
      size = toInt(arg);
    }

    if (offset + size > maxSize) return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));

    data.append((const char *)(t + offset), size);
  }

  return makeBinaryReply(data);
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

  std::vector<Glyph>    glyphs;
  std::vector<Index>    index;
  std::vector<uint8_t>  bitmapdata;
  qint32                stride;

  auto readMetrics = [&glyphs](QByteArray section) {
    QDataStream in(&section, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    qint32 format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(QDataStream::BigEndian);
    }

    if (format & PCF_COMPRESSED_METRICS) {
      qint16 count;
      in >> count;

      glyphs.resize(count);
      for (qint16 i = 0; i < count; i++) {
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
      qint16 count;
      in >> count;

      glyphs.resize(count);
      for (qint16 i = 0; i < count; i++) {
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

  auto readBitmaps = [&glyphs, &bitmapdata](QByteArray section, QByteArray file) {
    QDataStream in(&section, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    qint32 format;
    in >> format;

    if (format & PCF_BYTE_MASK) {
      in.setByteOrder(QDataStream::BigEndian);
    }

    quint32 count;
    in >> count;

    std::vector<quint32> offsets;
    offsets.reserve(count);
    for (quint32 i = 0; i < count; i++) {
      quint32 offset;
      in >> offset;

      offsets.push_back(offset);
    }

    quint32 bitmapSizes[4];
    for (quint32 i = 0; i < 4; i++) {
      in >> bitmapSizes[i];
    }

    quint32 bitmapSize = bitmapSizes[format & 3];

    // read bitmap data:
    glyphs.resize(count);
    for (quint32 i = 0; i < count; i++) {
      glyphs[i].m_bitmapdata.resize(bitmapSize);
      memcpy(glyphs[i].m_bitmapdata.data(), file.constData() + offsets[i], bitmapSize);
    }
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
          //readEncodings(data.mid(offset, size));
          break;
      }
    }

    // add in new font at requested index:
    wasmInterface.fonts.resize(fontindex+1);
    wasmInterface.fonts[fontindex].reset(
      new Font(glyphs, index, stride)
    );

    return makeOkReply();
  };

  return readPCF();
}
