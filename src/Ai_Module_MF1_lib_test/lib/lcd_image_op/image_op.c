#include "image_op.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// RGB888 顺时针 90 度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb888_roate_right90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for(uint32_t i = 0; i < w; i++)
    {
        for(uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0)) * 3 + 0) = *(src + ((h - 1 - (j * 2 + 0)) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 0)) * 3 + 1) = *(src + ((h - 1 - (j * 2 + 0)) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 0)) * 3 + 2) = *(src + ((h - 1 - (j * 2 + 0)) * w + i) * 3 + 2);

            *(out + (i * h + (j * 2 + 1)) * 3 + 0) = *(src + ((h - 1 - (j * 2 + 1)) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 1)) * 3 + 1) = *(src + ((h - 1 - (j * 2 + 1)) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 1)) * 3 + 2) = *(src + ((h - 1 - (j * 2 + 1)) * w + i) * 3 + 2);
        }
    }
    return;
}

// RGB888 逆时针90度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb888_roate_left90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for(uint32_t i = 0; i < w; i++)
    {
        for(uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0)) * 3 + 0) = *(src + ((j * 2 + 0) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 0)) * 3 + 1) = *(src + ((j * 2 + 0) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 0)) * 3 + 2) = *(src + ((j * 2 + 0) * w + i) * 3 + 2);

            *(out + (i * h + (j * 2 + 1)) * 3 + 0) = *(src + ((j * 2 + 1) * w + i) * 3 + 0);
            *(out + (i * h + (j * 2 + 1)) * 3 + 1) = *(src + ((j * 2 + 1) * w + i) * 3 + 1);
            *(out + (i * h + (j * 2 + 1)) * 3 + 2) = *(src + ((j * 2 + 1) * w + i) * 3 + 2);
        }
    }
    return;
}

//FIXME: 需要调用 convert_rgb565_order
// RGB565 顺时针 90 度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb565_roate_right90(uint16_t *out, uint16_t *src, uint16_t w, uint16_t h)
{
    for(uint32_t i = 0; i < w; i++)
    {
        for(uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((h - 1 - (j * 2 + 0)) * w + i));

            *(out + (i * h + (j * 2 + 1))) = *(src + ((h - 1 - (j * 2 + 1)) * w + i));
        }
    }
    return;
}

void convert_rgb565_order(uint16_t *image, uint16_t w, uint16_t h)
{
    uint16_t tmp;

    for(uint16_t i = 0; i < h; i++)
    {
        for(uint16_t j = 0; j < (w / 2); j++)
        {
            tmp = *(image + i * w + j * 2 + 0);
            *(image + i * w + j * 2 + 0) = *(image + i * w + j * 2 + 1);
            *(image + i * w + j * 2 + 1) = tmp;
        }
    }
}

//FIXME: 需要调用 convert_rgb565_order
// RGB565 逆时针90度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_rgb565_roate_left90(uint16_t *out, uint16_t *src, uint16_t w, uint16_t h)
{
    for(uint32_t i = 0; i < w; i++)
    {
        for(uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((j * 2 + 0) * w + i));

            *(out + (i * h + (j * 2 + 1))) = *(src + ((j * 2 + 1) * w + i));
        }
    }
    return;
}

// R8G8B8 顺时针 90 度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_r8g8b8_roate_right90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for(uint32_t i = 0; i < w; i++)
    {
        for(uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((h - 1 - (j * 2 + 0)) * w + i));
            *(out + (i * h + (j * 2 + 1))) = *(src + ((h - 1 - (j * 2 + 1)) * w + i));
        }
    }
    return;
}

// R8G8B8 逆时针90度
// 注意旋转90 后，图像数据的宽高会对调，显示时候自己注意
// !!! 宽度及高度必须是2的倍数
void image_r8g8b8_roate_left90(uint8_t *out, uint8_t *src, uint16_t w, uint16_t h)
{
    for(uint32_t i = 0; i < w; i++)
    {
        for(uint32_t j = 0; j < (h / 2); j++)
        {
            *(out + (i * h + (j * 2 + 0))) = *(src + ((j * 2 + 0) * w + i));
            *(out + (i * h + (j * 2 + 1))) = *(src + ((j * 2 + 1) * w + i));
        }
    }
    return;
}

void face_obj_info_roate_right_90(face_obj_info_t *face_obj, uint16_t w, uint16_t h)
{
    face_obj_t *obj = NULL;
    uint16_t x1, y1, x2, y2;

    for(uint32_t i = 0; i < (face_obj->obj_number); i++)
    {
        obj = &(face_obj->obj[i]);

        //copy origin data
        x1 = obj->x1;
        y1 = obj->y1;
        x2 = obj->x2;
        y2 = obj->y2;

        //roate right 90
        obj->x1 = h - 1 - y2;
        obj->y1 = x1;
        obj->x2 = h - 1 - y1;
        obj->y2 = x2;
    }
}

void face_obj_info_roate_left_90(face_obj_info_t *face_obj, uint16_t w, uint16_t h)
{
    face_obj_t *obj = NULL;
    uint16_t x1, y1, x2, y2;

    for(uint32_t i = 0; i < (face_obj->obj_number); i++)
    {
        obj = &(face_obj->obj[i]);

        //copy origin data
        x1 = obj->x1;
        y1 = obj->y1;
        x2 = obj->x2;
        y2 = obj->y2;

        //roate left 90
        obj->x1 = y1;
        obj->y1 = w - 1 - x2;
        obj->x2 = y2;
        obj->y2 = w - 1 - x1;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int stat_rect(image_t *image,
              uint16_t x0, uint16_t y0,
              uint16_t x1, uint16_t y1,
              uint16_t stride)
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

void rgb5652rgb888(uint16_t *rgb565, uint8_t *rgb888,
                   uint16_t img_w, uint16_t img_h)
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
/* clang-format off */
#define RGB888_RED          (0x00ff0000)
#define RGB888_GREEN        (0x0000ff00)
#define RGB888_BLUE         (0x000000ff)
/* clang-format on */

uint16_t rgb8882rgb565(uint32_t rgb888)
{
    uint8_t cRed = (rgb888 & RGB888_RED) >> 19;
    uint8_t cGreen = (rgb888 & RGB888_GREEN) >> 10;
    uint8_t cBlue = (rgb888 & RGB888_BLUE) >> 3;

    uint16_t n565Color = (uint16_t)((cRed << 11) + (cGreen << 5) + (cBlue << 0));
    return n565Color;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void image_rgb565_draw_edge(uint32_t *gram, face_obj_t *obj,
                            uint16_t color, uint16_t img_w, uint16_t img_h)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4, x1, y1, x2, y2;

    x1 = obj->x1;
    y1 = obj->y1;
    x2 = obj->x2;
    y2 = obj->y2;

    if(x1 <= 0)
        x1 = 1;
    if(x2 >= img_w - 1)
        x2 = img_w - 2;
    if(y1 <= 0)
        y1 = 1;
    if(y2 >= img_h - 1)
        y2 = img_h - 2;

    addr1 = gram + (img_w * y1 + x1) / 2;
    addr2 = gram + (img_w * y1 + x2 - 8) / 2;
    addr3 = gram + (img_w * (y2 - 1) + x1) / 2;
    addr4 = gram + (img_w * (y2 - 1) + x2 - 8) / 2;

    for(uint32_t i = 0; i < 4; i++)
    {
        *addr1 = data;
        *(addr1 + img_w / 2) = data;
        *addr2 = data;
        *(addr2 + img_w / 2) = data;
        *addr3 = data;
        *(addr3 + img_w / 2) = data;
        *addr4 = data;
        *(addr4 + img_w / 2) = data;
        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }

    addr1 = gram + (img_w * y1 + x1) / 2;
    addr2 = gram + (img_w * y1 + x2 - 2) / 2;
    addr3 = gram + (img_w * (y2 - 8) + x1) / 2;
    addr4 = gram + (img_w * (y2 - 8) + x2 - 2) / 2;

    for(uint32_t i = 0; i < 8; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;
        addr1 += img_w / 2;
        addr2 += img_w / 2;
        addr3 += img_w / 2;
        addr4 += img_w / 2;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern uint8_t const ascii0816[];

static void image_rgb565_ram_draw_char(uint32_t *ptr, char c,
                                       uint16_t x, uint16_t y,
                                       uint16_t color, uint16_t *bg_color,
                                       uint16_t img_w)
{
    uint8_t i, j, data;
    uint16_t *addr;

    for(i = 0; i < 16; i++)
    {
        addr = ((uint16_t *)ptr) + y * (img_w + 0) + x;
        data = ascii0816[c * 16 + i];
        for(j = 0; j < 8; j++)
        {
            if(data & 0x80)
            {
                if((x + j) & 1)
                    *(addr - 1) = color;
                else
                    *(addr + 1) = color;
            } else
            {
                if(bg_color)
                {
                    if((x + j) & 1)
                        *(addr - 1) = *bg_color;
                    else
                        *(addr + 1) = *bg_color;
                }
            }

            data <<= 1;
            addr++;
        }
        y++;
    }
}

void image_rgb565_draw_string(uint32_t *ptr, char *str,
                              uint16_t x, uint16_t y,
                              uint16_t color, uint16_t *bg_color,
                              uint16_t img_w)
{
    while(*str)
    {
        image_rgb565_ram_draw_char(ptr, *str, x, y, color, bg_color, img_w);
        str++;
        x += 8;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint16_t Fast_AlphaBlender(uint32_t x, uint32_t y, uint32_t Alpha)
{
    x = (x | (x << 16)) & 0x7E0F81F;
    y = (y | (y << 16)) & 0x7E0F81F;
    uint32_t result = ((((x - y) * Alpha) >> 5) + y) & 0x7E0F81F;
    return (uint16_t)((result & 0xFFFF) | (result >> 16));
}

void image_rgb565_mix_pic_with_alpha(mix_image_t *img_src, mix_image_t *img_dst, uint32_t alpha)
{
    uint16_t sx, sy, sw, sh;
    uint16_t dx, dy, dw, dh;

    sx = img_src->x;
    sy = img_src->y;
    sw = img_src->w;
    sh = img_src->h;

    dx = img_dst->x;
    dy = img_dst->y;
    dw = img_dst->w;
    dh = img_dst->h;

    if((dx + dw) > (sw + sx) || (dh + dy) > (sh + sy))
    {
        printf("[image_rgb565_mix_pic_with_alpha]:image invaild\r\n");
        return;
    }

    for(uint16_t i = dy; i < (dy + dh); i++)
    {
        for(uint16_t j = dx; j < (dx + dw); j++)
        {
            *(img_src->img_addr + i * sw + j) = Fast_AlphaBlender((uint32_t) * (img_src->img_addr + i * sw + j),
                                                                  (uint32_t) * (img_dst->img_addr + (i - dy) * dw + (j - dx)),
                                                                  (uint32_t)alpha / 8);
        }
    }
    return;
}

//等待添加显示中文字符串，以及字号选择
