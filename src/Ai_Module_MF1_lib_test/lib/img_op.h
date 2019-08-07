#ifndef __IMG_OP_H
#define __IMG_OP_H

#include "face_lib.h"

int stat_rect(image_t *image, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t stride);
void rgb5652rgb888(uint16_t *rgb565, uint8_t *rgb888, uint16_t img_w, uint16_t img_h);


#endif
