
struct ppux_sprite {
  uint32_t index;
  uint8_t  enabled;

  uint16_t x;
  uint16_t y;
  uint8_t  hflip;
  uint8_t  vflip;

  uint8_t  vram_space;        //     0 ..   255; 0 = local, 1..255 = extra
  uint16_t vram_addr;         // $0000 .. $FFFF (byte address)
  uint8_t  cgram_space;       //     0 ..   255; 0 = local, 1..255 = extra
  uint8_t  palette;           //     0 ..   255

  uint8_t  layer;             // 0.. 4;  BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4
  uint8_t  priority;          // 1..12
  uint8_t  color_exemption;     // true = ignore color math, false = obey color math

  uint8_t  bpp;               // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
  uint16_t width;             // number of pixels width
  uint16_t height;            // number of pixels high
};

QByteArray NWAccess::cmdWasmReset(QByteArray args)
{
  WASM::host.reset();
  return makeOkReply();
}

QByteArray NWAccess::cmdWasmAdd(QByteArray args, QByteArray data)
{
  QByteArray reply;

  try {
    WASM::host.add_module(reinterpret_cast<const uint8_t *>(data.constData()), data.size());

    WASM::host.linkEx("*", "ppux_reset", "v()", &WASM::RawCall<NWAccess>::adapter<&NWAccess::wasm_ppux_reset>, (const void *)this);
    WASM::host.linkEx("*", "ppux_sprite_write", "i(*)", &WASM::RawCall<NWAccess>::adapter<&NWAccess::wasm_ppux_sprite_write>, (const void *)this);

    reply = makeOkReply();
  } catch (WASM::error& err) {
    reply = makeErrorReply(err.what());
  }

  return reply;
}

QByteArray NWAccess::cmdWasmInvoke(QByteArray args)
{
  QStringList items = QString::fromUtf8(args).split(';');

  QString reply = "name:";
  reply += items[0];

  try {
    const char **argv = nullptr;
    size_t count = items.size() - 1;
    if (count > 0) {
      argv = new const char *[count];
      for (unsigned i = 0; i < count; i++) {
        QString item = items.at(i + 1);
        reply += QString("\np%1:%2").arg(i).arg(item);

        argv[i] = item.toUtf8().constData();
      }
    }

    QString function_name_qs = items[0];
    QByteArray function_name_qba = function_name_qs.toUtf8();
    const char *function_name = function_name_qba.constData();

    WASM::host.invoke_all(function_name, count, argv);

    if (argv) delete[] argv;
  } catch (WASM::error& err) {
    if (!reply.isEmpty()) reply.append('\n');
    reply += "error:";
    reply += err.what();
  }

  return makeHashReply(reply);
}

m3ApiRawFunction(NWAccess::wasm_ppux_reset)
{
  SNES::ppu.ppuxReset();
  m3ApiSuccess();
}

m3ApiRawFunction(NWAccess::wasm_ppux_sprite_write) {
  m3ApiReturnType(int32_t)

  m3ApiGetArgMem(struct ppux_sprite *, i_spr)

  if (!i_spr) {
    m3ApiReturn(-1);
  }

  if (i_spr->index >= SNES::PPU::extra_count) {
    m3ApiReturn(-2);
  }

  if (i_spr->vram_space >= SNES::PPU::extra_spaces) {
    m3ApiReturn(-3);
  }

  if (i_spr->cgram_space >= SNES::PPU::extra_spaces) {
    m3ApiReturn(-4);
  }

  auto &t = SNES::ppu.extra_list[i_spr->index];

  t.enabled = i_spr->enabled;

  t.x = i_spr->x;
  t.y = i_spr->y;
  t.hflip = i_spr->hflip;
  t.vflip = i_spr->vflip;

  t.vram_space = i_spr->vram_space;
  t.vram_addr = i_spr->vram_addr;
  t.cgram_space = i_spr->cgram_space;
  t.palette = i_spr->palette;

  t.layer = i_spr->layer;
  t.priority = i_spr->priority;
  t.color_exemption = i_spr->color_exemption;

  t.bpp = i_spr->bpp;
  t.width = i_spr->width;
  t.height = i_spr->height;

  m3ApiSuccess();
}

m3ApiRawFunction(NWAccess::wasm_ppux_ram_write) {
  m3ApiSuccess();
}

m3ApiRawFunction(NWAccess::wasm_ppux_ram_read) {
  m3ApiSuccess();
}
