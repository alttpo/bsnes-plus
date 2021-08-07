#include "drawing.h"

__attribute__((export_name("draw_hline")))
void draw_hline(struct frame *frame, int32_t x0, int32_t y0, int32_t w, uint16_t color) {
  if (y0 < 0) return;
  if (y0 >= frame->height) return;

  for (int32_t x = x0; x < x0+w; x++) {
    if (x < 0) continue;
    if (x >= frame->width) continue;

    frame->data[y0 * frame->pitch + x] = color;
  }
}
