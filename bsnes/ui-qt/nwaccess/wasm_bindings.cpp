
struct ppux_sprite {
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

enum memory_type : uint32_t {
  VRAM,
  CGRAM
};

const char *NWAccess::wasmsig_ppux_reset = "v()";
m3ApiRawFunction(NWAccess::wasm_ppux_reset)
{
  SNES::ppu.ppuxReset();
  m3ApiSuccess();
}

const char *NWAccess::wasmsig_ppux_sprite_write = "i(i*)";
m3ApiRawFunction(NWAccess::wasm_ppux_sprite_write) {
  m3ApiReturnType(int32_t)

  m3ApiGetArg(uint32_t, i_index)
  m3ApiGetArgMem(struct ppux_sprite *, i_spr)

  if (i_index >= SNES::PPU::extra_count) {
    m3ApiReturn(-1);
  }

  if (!i_spr) {
    m3ApiReturn(-2);
  }

  if (i_spr->vram_space >= SNES::PPU::extra_spaces) {
    m3ApiReturn(-3);
  }

  if (i_spr->cgram_space >= SNES::PPU::extra_spaces) {
    m3ApiReturn(-4);
  }

  auto &t = SNES::ppu.extra_list[i_index];

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

  m3ApiReturn(0);
}

const char *NWAccess::wasmsig_ppux_ram_write = "i(iii*i)";
m3ApiRawFunction(NWAccess::wasm_ppux_ram_write) {
  m3ApiReturnType(int32_t)

  m3ApiGetArg   (memory_type, i_memory)
  m3ApiGetArg   (uint32_t,    i_space)
  m3ApiGetArg   (uint32_t,    i_offset)
  m3ApiGetArgMem(uint8_t*,    i_data)
  m3ApiGetArg   (uint32_t,    i_size)

  if (i_space >= SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  if (i_memory == memory_type::VRAM) {
    t = SNES::ppu.get_vram_space(i_space);
    maxSize = 0x10000;
  } else if (i_memory == memory_type::CGRAM) {
    t = SNES::ppu.get_cgram_space(i_space);
    maxSize = 0x200;
  } else {
    //return makeErrorReply(QString("unknown memory type '%1' expected 'VRAM' or 'CGRAM'").arg(memory));
    m3ApiReturn(-1);
  }
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(t + i_offset, i_data, i_size);

  m3ApiReturn(0);
}

const char *NWAccess::wasmsig_ppux_ram_read = "i(iii*i)";
m3ApiRawFunction(NWAccess::wasm_ppux_ram_read) {
  m3ApiReturnType(int32_t)

  m3ApiGetArg   (memory_type, i_memory)
  m3ApiGetArg   (uint32_t,    i_space)
  m3ApiGetArg   (uint32_t,    i_offset)
  m3ApiGetArgMem(uint8_t*,    o_data)
  m3ApiGetArg   (uint32_t,    i_size)

  if (i_space >= SNES::PPU::extra_spaces) {
    //return makeErrorReply(QString("space must be 0..%1").arg(SNES::PPU::extra_spaces-1));
    m3ApiReturn(-2);
  }

  unsigned maxSize = 0;
  uint8_t *t = nullptr;
  if (i_memory == memory_type::VRAM) {
    t = SNES::ppu.get_vram_space(i_space);
    maxSize = 0x10000;
  } else if (i_memory == memory_type::CGRAM) {
    t = SNES::ppu.get_cgram_space(i_space);
    maxSize = 0x200;
  } else {
    //return makeErrorReply(QString("unknown memory type '%1' expected 'VRAM' or 'CGRAM'").arg(memory));
    m3ApiReturn(-1);
  }
  if (!t) {
    //return makeErrorReply(QString("%1 memory not allocated for space %2").arg(memory).arg(space));
    m3ApiReturn(-1);
  }

  if (i_offset >= maxSize) {
    //return makeErrorReply(QString("offset must be 0..$%1").arg(maxSize-1, 0, 16));
    m3ApiReturn(-3);
  }
  if (i_offset & 1) {
    //return makeErrorReply("offset must be multiple of 2");
    m3ApiReturn(-4);
  }

  if (i_offset + i_size > maxSize) {
    //return makeErrorReply(QString("offset+size must be 0..$%1, offset+size=$%2").arg(maxSize, 0, 16).arg(offset+size, 0, 16));
    m3ApiReturn(-5);
  }

  memcpy(o_data, t + i_offset, i_size);

  m3ApiReturn(0);
}
