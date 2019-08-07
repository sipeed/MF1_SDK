#include "img_op.h"

extern uint8_t mux_id;
extern uint8_t kpu_image[2][IMG_W * IMG_H * 3];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int stat_rect(image_t *image, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t stride)
{
    int cnt = 0;
    uint32_t sum_gray = 0;
    int gray, r, g, b, x, y;
    int min_gray = 255;
    int max_gray = 0;
    int min_gray_x = 0, min_gray_y = 0;
    int w = image->width;
    int h = image->height;
    uint8_t *img = image->addr;
    uint8_t *R = img + 0 * w * h;
    uint8_t *G = img + 1 * w * h;
    uint8_t *B = img + 2 * w * h;
    int oft;
    for(y = y0; y < y1; y += stride)
    {
        for(x = x0; x < x1; x += stride)
        {
            oft = y * w + x;
            r = R[oft];
            g = G[oft];
            b = B[oft];
            gray = (r * 76 + g * 150 + b * 30) / 256;
            sum_gray += gray;
            cnt++;
        }
    }
    return (sum_gray / cnt);
}

/* clang-format off */
#define RGB565_RED          (0xf800)
#define RGB565_GREEN        (0x07e0)
#define RGB565_BLUE         (0x001f)
/* clang-format on */

void rgb5652rgb888(uint16_t *rgb565, uint8_t *rgb888, uint16_t img_w, uint16_t img_h)
{
    uint16_t pixel = 0;

    for(uint16_t i = 0; i < img_h; i++)
    {
        for(uint16_t j = 0; j < img_w / 2; j++)
        {
            pixel = (uint16_t) * (rgb565 + i * img_w + (j * 2 + 1));
            *(rgb888 + ((j * 2) + i * img_w) * 3 + 0) = (pixel & RGB565_RED) >> 8;
            *(rgb888 + ((j * 2) + i * img_w) * 3 + 1) = (pixel & RGB565_GREEN) >> 3;
            *(rgb888 + ((j * 2) + i * img_w) * 3 + 2) = (pixel & RGB565_BLUE) << 3;

            pixel = (uint16_t) * (rgb565 + i * img_w + (j * 2));
            *(rgb888 + ((j * 2 + 1) + i * img_w) * 3 + 0) = (pixel & RGB565_RED) >> 8;
            *(rgb888 + ((j * 2 + 1) + i * img_w) * 3 + 1) = (pixel & RGB565_GREEN) >> 3;
            *(rgb888 + ((j * 2 + 1) + i * img_w) * 3 + 2) = (pixel & RGB565_BLUE) << 3;
        }
    }
}
