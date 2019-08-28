#ifndef __IMAGE_OP_H
#define __IMAGE_OP_H

#include <stdint.h>
#include "face_lib.h"

void image_rgb888_roate_right90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h);
void image_rgb888_roate_left90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h);

void image_rgb565_roate_right90(uint16_t *out, uint16_t *src, uint16_t w, uint16_t h);
void image_rgb565_roate_left90(uint16_t *out, uint16_t *src, uint16_t w, uint16_t h);

void image_r8g8b8_roate_right90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h);
void image_r8g8b8_roate_left90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h);

void convert_rgb565_order(uint16_t *image, uint16_t w, uint16_t h);

void face_obj_info_roate_right_90(face_obj_info_t *face_obj, uint16_t w, uint16_t h);
void face_obj_info_roate_left_90(face_obj_info_t *face_obj, uint16_t w, uint16_t h);

int stat_rect(image_t *image,
              uint16_t x0, uint16_t y0,
              uint16_t x1, uint16_t y1,
              uint16_t stride);

void rgb5652rgb888(uint16_t *rgb565, uint8_t *rgb888,
                   uint16_t img_w, uint16_t img_h);

uint16_t rgb8882rgb565(uint32_t rgb888);

void image_rgb565_draw_edge(uint32_t *gram, face_obj_t *obj,
                            uint16_t color, uint16_t img_w, uint16_t img_h);

void image_rgb565_draw_string(uint32_t *ptr, char *str,
                              uint16_t x, uint16_t y,
                              uint16_t color, uint16_t *bg_color,
                              uint16_t img_w);

typedef struct
{
    uint16_t *img_addr;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} mix_image_t;

void image_rgb565_mix_pic_with_alpha(mix_image_t *img_src, mix_image_t *img_dst, uint32_t alpha);

#endif
