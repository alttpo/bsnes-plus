
// libc: (requires -fno-builtin)

__attribute__((import_module("env"), import_name("_memset")))
void *memset(void *data, int c, unsigned long len);

__attribute__((import_module("env"), import_name("_memcpy")))
void *memcpy(void *data, const void *src, unsigned long len);

__attribute__((import_module("env"), import_name("puts")))
int32_t puts(const char *i_str);

__attribute__((import_module("env"), import_name("hexdump")))
void hexdump(const uint8_t *i_data, uint32_t i_size);

typedef enum log_level {
  L_DEBUG,
  L_INFO,
  L_WARN,
  L_ERROR
} log_level;

__attribute__((import_module("env"), import_name("log_c")))
void log(log_level level, const char* msg);


// ppux:

enum draw_color_kind {
  COLOR_STROKE,
  COLOR_FILL,
  COLOR_OUTLINE,

  COLOR_MAX
};

const uint16_t color_none = 0x8000;

enum draw_cmd : uint16_t {
  // commands which affect state:
  ///////////////////////////////
  CMD_TARGET = 1,
  CMD_COLOR_DIRECT_BGR555,
  CMD_COLOR_DIRECT_RGB888,
  CMD_COLOR_PALETTED,
  CMD_FONT_SELECT,

  // commands which use state:
  ///////////////////////////////
  CMD_TEXT_UTF8 = 0x40,
  CMD_PIXEL,
  CMD_HLINE,
  CMD_VLINE,
  CMD_LINE,
  CMD_RECT,
  CMD_RECT_FILL,

  // commands which ignore state:
  ///////////////////////////////
  CMD_VRAM_TILE = 0x80,
  CMD_IMAGE,
};

enum draw_layer : uint16_t {
  BG1 = 0,
  BG2 = 1,
  BG3 = 2,
  BG4 = 3,
  OAM = 4,
  BACK = 5,
  COL = 5
};

__attribute__((import_module("env"), import_name("za_file_locate_c")))
int32_t za_file_locate(const char *i_filename, uint32_t* o_fh);

__attribute__((import_module("env"), import_name("za_file_size")))
int32_t za_file_size(int32_t fh, uint64_t* o_size);

__attribute__((import_module("env"), import_name("za_file_extract")))
int32_t za_file_extract(int32_t fh, void *o_data, uint64_t i_size);

__attribute__((import_module("env"), import_name("ppux_font_load_za")))
int32_t ppux_font_load_za(int32_t i_fontindex, int32_t i_za_fh);

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
int32_t ppux_oam_write(uint32_t i_offset, uint8_t *i_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_vram_read")))
int32_t ppux_vram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_cgram_read")))
int32_t ppux_cgram_read(uint32_t i_space, uint32_t i_offset, uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_oam_read")))
int32_t ppux_oam_read(uint32_t i_offset, uint8_t *o_data, uint32_t i_size);

__attribute__((import_module("env"), import_name("ppux_draw_list_clear")))
void ppux_draw_list_clear();

__attribute__((import_module("env"), import_name("ppux_draw_list_resize")))
void ppux_draw_list_resize(uint32_t i_len);

__attribute__((import_module("env"), import_name("ppux_draw_list_set")))
uint32_t ppux_draw_list_set(uint32_t i_index, uint32_t i_len, uint16_t* i_cmdlist);

__attribute__((import_module("env"), import_name("ppux_draw_list_append")))
uint32_t ppux_draw_list_append(uint32_t i_len, uint16_t* i_cmdlist);

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
