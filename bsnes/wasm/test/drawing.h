#pragma once

#include <stdint.h>

struct frame {
  uint16_t *data;
  uint32_t pitch;
  uint32_t width;
  uint32_t height;
  int32_t  interlace;
};

__attribute__((import_module("drawing"), import_name("draw_hline")))
void draw_hline(struct frame *frame, int32_t x0, int32_t y0, int32_t w, uint16_t color);
