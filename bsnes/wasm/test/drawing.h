#pragma once

#include <stdint.h>

enum draw_cmd {
  CMD_PIXEL,
  CMD_IMAGE,
  CMD_HLINE,
  CMD_VLINE,
  CMD_RECT,
  CMD_TEXT_UTF8,
  CMD_VRAM_TILE_2BPP,
  CMD_VRAM_TILE_4BPP,
  CMD_VRAM_TILE_8BPP
};

__attribute__((import_module("env"), import_name("draw_list")))
void draw_list(uint32_t size, uint16_t* cmdlist);
