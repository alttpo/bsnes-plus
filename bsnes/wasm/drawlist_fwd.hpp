#pragma once

namespace DrawList {

enum draw_color_kind {
  COLOR_STROKE,
  COLOR_FILL,
  COLOR_OUTLINE,

  COLOR_MAX
};

const uint16_t color_none = 0x8000;

// default text alignment is {left, top}
enum text_alignment {
  TEXT_HALIGN_LEFT   = 0,
  TEXT_HALIGN_CENTER = 1 << 0,
  TEXT_HALIGN_RIGHT  = 1 << 1,
  TEXT_VALIGN_TOP    = 0,
  TEXT_VALIGN_MIDDLE = 1 << 2,
  TEXT_VALIGN_BOTTOM = 1 << 3,
};

enum draw_cmd : uint16_t {
  // commands which affect state:
  ///////////////////////////////

  // 4, CMD_TARGET, draw_layer, pre_mode7_transform, priority
  CMD_TARGET = 1,
  CMD_COLOR_DIRECT_BGR555,
  CMD_COLOR_DIRECT_RGB888,
  CMD_COLOR_PALETTED,
  CMD_FONT_SELECT,
  CMD_TEXT_ALIGN,

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

struct FontContainer;

struct Space;

struct LocalSpace;

struct ExtraSpace;

struct SpaceContainer;

struct Renderer;

struct Context;

}
