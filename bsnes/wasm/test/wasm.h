
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

enum draw_cmd {
  CMD_PIXEL,
  CMD_IMAGE,
  CMD_HLINE,
  CMD_VLINE,
  CMD_RECT,
  CMD_TEXT_UTF8,
  CMD_VRAM_TILE
};

__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

__attribute__((import_module("env"), import_name("ppux_vram_reset")))
void ppux_vram_reset();

__attribute__((import_module("env"), import_name("ppux_cgram_reset")))
void ppux_cgram_reset();

__attribute__((import_module("env"), import_name("ppux_vram_write")))
int32_t ppux_vram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data,
                       uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_draw_list_reset")))
void ppux_draw_list_reset();

__attribute__((import_module("env"), import_name("ppux_draw_list_append")))
void ppux_draw_list_append(uint8_t layer, uint8_t priority, uint32_t size, uint16_t* cmdlist);

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

// debugger:

__attribute__((import_module("env"), import_name("debugger_break")))
void debugger_break();

__attribute__((import_module("env"), import_name("debugger_continue")))
void debugger_continue();
