#pragma once

#include <stdint.h>

struct frame {
  uint32_t pitch;
  uint32_t width;
  uint32_t height;
  int32_t  interlace;
  uint16_t data[512 * 512];
};

__attribute__((import_module("env"), import_name("frame_acquire")))
void frame_acquire(struct frame* io_frame);

__attribute__((import_module("env"), import_name("draw_hline")))
void draw_hline(struct frame *io_frame, int32_t x0, int32_t y0, int32_t w, uint16_t color);
