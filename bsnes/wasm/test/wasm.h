
// libc: (requires -fno-builtin)

__attribute__((import_module("env"), import_name("_memset")))
void *memset(void *data, int c, unsigned long len);

__attribute__((import_module("env"), import_name("_memcpy")))
void *memcpy(void *data, const void *src, unsigned long len);

__attribute__((import_module("env"), import_name("puts")))
int32_t puts(const char *i_str);

__attribute__((import_module("env"), import_name("hexdump")))
void hexdump(const uint8_t *i_data, uint32_t i_size);


// ppux:

struct ppux_sprite {
  uint8_t enabled;

  uint16_t x;
  uint16_t y;
  uint8_t hflip;
  uint8_t vflip;

  uint8_t vram_space;        //     0 ..   255; 0 = local, 1..255 = extra
  uint16_t vram_addr;         // $0000 .. $FFFF (byte address)
  uint8_t cgram_space;       //     0 ..   255; 0 = local, 1..255 = extra
  uint8_t palette;           //     0 ..   255

  uint8_t layer;             // 0.. 4;  BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4
  uint8_t priority;          // 1..12
  uint8_t color_exemption;   // true = ignore color math, false = obey color math

  uint8_t bpp;               // 2-, 4-, or 8-bpp tiles from vram[extra] and cgram[extra]
  uint16_t width;             // number of pixels width
  uint16_t height;            // number of pixels high
};

enum ppux_memory_type : uint32_t {
  VRAM,
  CGRAM
};

__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_reset")))
void ppux_sprite_reset();

__attribute__((import_module("env"), import_name("ppux_sprite_read")))
int32_t ppux_sprite_read(uint32_t i_index, struct ppux_sprite *o_spr);

__attribute__((import_module("env"), import_name("ppux_sprite_write")))
int32_t ppux_sprite_write(uint32_t i_index, struct ppux_sprite *i_spr);

__attribute__((import_module("env"), import_name("ppux_ram_write")))
int32_t ppux_ram_write(enum ppux_memory_type i_memorytype, uint32_t i_space, uint32_t i_offset, uint8_t *i_data,
                       uint32_t i_size);

// snes:

__attribute__((import_module("env"), import_name("snes_bus_read")))
void snes_bus_read(uint32_t i_address, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("snes_bus_write")))
void snes_bus_write(uint32_t i_address, uint8_t *o_data, uint32_t i_size);

// msg:

__attribute__((import_module("env"), import_name("msg_recv")))
int32_t msg_recv(uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("msg_size")))
int32_t msg_size(uint16_t *o_size);
