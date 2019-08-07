#ifndef __DRAW_H__
#define __DRAW_H__
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include "fmath.h"

#define IM_MAX(a,b)     ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define IM_MIN(a,b)     ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define IM_DIV(a,b)     ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _b ? (_a / _b) : 0; })
#define IM_MOD(a,b)     ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _b ? (_a % _b) : 0; })

#define IM_DEG2RAD(x)   (((x)*M_PI)/180)
#define IM_RAD2DEG(x)   (((x)*180)/M_PI)

#define IMAGE_GET_RGB565_PIXEL(image, x, y) \
({ \
    __typeof__ (image) _image = (image); \
    __typeof__ (x) _x = (x); \
    __typeof__ (y) _y = (y); \
    ((uint16_t *) _image->data)[(_image->w * _y) + _x]; \
})

#define IMAGE_PUT_RGB565_PIXEL(image, x, y, v) \
({ \
    __typeof__ (image) _image = (image); \
    __typeof__ (x) _x = (x); \
    __typeof__ (y) _y = (y); \
    __typeof__ (v) _v = (v); \
    ((uint16_t *) _image->data)[(_image->w * _y) + _x] = _v; \
})

#define COLOR_Y_MIN -128
#define COLOR_Y_MAX 127
#define COLOR_U_MIN -128
#define COLOR_U_MAX 127
#define COLOR_V_MIN -128
#define COLOR_V_MAX 127

#define COLOR_RGB565_TO_Y(pixel) yuv_table((pixel) * 3)
#define COLOR_RGB565_TO_U(pixel) yuv_table(((pixel) * 3) + 1)
#define COLOR_RGB565_TO_V(pixel) yuv_table(((pixel) * 3) + 2)

#define COLOR_RGB565_TO_BINARY(pixel) (COLOR_RGB565_TO_Y(pixel) > (((COLOR_Y_MAX - COLOR_Y_MIN) / 2) + COLOR_Y_MIN))

//#define PI M_PI

typedef enum image_bpp
{
    IMAGE_BPP_BINARY,       // BPP = 0
    IMAGE_BPP_GRAYSCALE,    // BPP = 1
    IMAGE_BPP_RGB565,       // BPP = 2
    IMAGE_BPP_BAYER,        // BPP = 3
    IMAGE_BPP_JPEG,         // BPP = 4
}
image_bpp_t;

typedef struct image {
    int w;
    int h;
    int bpp;
    union {
        uint8_t *pixels;
        uint8_t *data;
    };
} __attribute__((aligned(8)))ui_image_t;

int imlib_get_pixel(ui_image_t *img, int x, int y);
void imlib_set_pixel(ui_image_t *img, int x, int y, int p);
void imlib_draw_line(ui_image_t *img, int x0, int y0, int x1, int y1, int c, int thickness);
void imlib_draw_rectangle(ui_image_t *img, int rx, int ry, int rw, int rh, int c, int thickness, bool fill);
void imlib_draw_circle(ui_image_t *img, int cx, int cy, int r, int c, int thickness, bool fill);
void imlib_draw_ellipse(ui_image_t *img, int cx, int cy, int rx, int ry, int rotation, int c, int thickness, bool fill);
void imlib_draw_string(ui_image_t *img, int x_off, int y_off, const char *str, int c, float scale, int x_spacing, int y_spacing, bool mono_space);
void imlib_draw_image(ui_image_t *img, ui_image_t *other, int x_off, int y_off, float x_scale, float y_scale, float alpha, ui_image_t *mask);
void imlib_flood_fill(ui_image_t *img, int x, int y,
                      float seed_threshold, float floating_threshold,
                      int c, bool invert, bool clear_background, ui_image_t *mask);
extern int8_t yuv_table(uint32_t idx);				  
#endif 
