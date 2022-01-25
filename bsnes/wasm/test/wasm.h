
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

enum draw_color_kind {
  COLOR_STROKE,
  COLOR_FILL,
  COLOR_OUTLINE,

  COLOR_MAX
};

const uint16_t color_none = 0x8000;

enum draw_cmd {
  // commands which ignore state:
  CMD_VRAM_TILE,
  CMD_IMAGE,
  // commands which affect state:
  CMD_COLOR_DIRECT_BGR555,
  CMD_COLOR_DIRECT_RGB888,
  CMD_COLOR_PALETTED,
  CMD_FONT_SELECT,
  // commands which use state:
  CMD_TEXT_UTF8,
  CMD_PIXEL,
  CMD_HLINE,
  CMD_VLINE,
  CMD_LINE,
  CMD_RECT,
  CMD_RECT_FILL
};

__attribute__((import_module("env"), import_name("za_file_locate")))
int32_t za_file_locate(const char *i_filename);

__attribute__((import_module("env"), import_name("za_file_size")))
int32_t za_file_size(int32_t fh, uint64_t* o_size);

__attribute__((import_module("env"), import_name("za_file_extract")))
int32_t za_file_extract(int32_t fh, void *o_data, uint64_t i_size);

__attribute__((import_module("env"), import_name("ppux_font_load_za")))
void ppux_font_load_za(int32_t i_fontindex, int32_t i_za_fh);

__attribute__((import_module("env"), import_name("ppux_font_delete")))
void ppux_font_delete(int32_t i_fontindex);

__attribute__((import_module("env"), import_name("ppux_vram_reset")))
void ppux_vram_reset();

__attribute__((import_module("env"), import_name("ppux_cgram_reset")))
void ppux_cgram_reset();

__attribute__((import_module("env"), import_name("ppux_vram_write")))
int32_t ppux_vram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_cgram_write")))
int32_t ppux_cgram_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_oam_write")))
int32_t ppux_oam_write(uint32_t i_space, uint32_t i_offset, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_vram_read")))
int32_t ppux_vram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_cgram_read")))
int32_t ppux_cgram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_oam_read")))
int32_t ppux_oam_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);

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
