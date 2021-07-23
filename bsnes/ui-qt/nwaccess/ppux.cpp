
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
