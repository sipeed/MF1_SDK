#ifndef __MAIX_QRCODE_H
#define __MAIX_QRCODE_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "yuv_tab.h"
///////////////////////////////////////////////////////////////////////////////
#define QUIRC_MAX_PAYLOAD 255

#define SWAP_16(x) ((x >> 8 & 0xff) | (x << 8))

#define COLOR_RGB565_TO_Y(pixel) yuv_table((pixel)*3)

#define COLOR_RGB565_TO_GRAYSCALE(pixel) (COLOR_RGB565_TO_Y(pixel) + 128)

#define IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x)   \
    ({                                            \
        __typeof__(row_ptr) _row_ptr = (row_ptr); \
        __typeof__(x) _x = (x);                   \
        _row_ptr[_x];                             \
    })

#define IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(image, y)   \
    ({                                                 \
        __typeof__(image) _image = (image);            \
        __typeof__(y) _y = (y);                        \
        ((uint16_t *)_image->data) + (_image->w * _y); \
    })

///////////////////////////////////////////////////////////////////////////////
enum enum_qrcode_res
{
    QRCODE_SUCC = 0,    /*  0: 扫码成功 */
    QRCODE_NONE = 1,    /* 1: 扫码失败 */
    QRCODE_ERROR = 2,   /* 出错 */
    QRCODE_TIMEOUT = 3, /* 2: 扫码超时 */
};

typedef struct _qrcode_image
{
    int w;
    int h;
    int bpp;
    union {
        uint8_t *pixels;
        uint8_t *data;
    };
} qrcode_image_t;

typedef struct _qrcode_scan
{
    int scan_time_out_s;
    uint64_t start_time_us;

    uint16_t img_w;
    uint16_t img_h;
    uint8_t *img_data;

    /* http://chicore.cn/doc/1180830026001202.htm */
    uint8_t qrcode_num;

    uint8_t qrcode[QUIRC_MAX_PAYLOAD];
} qrcode_scan_t;

///////////////////////////////////////////////////////////////////////////////
/* SDK中的函数 */
extern int printk(const char *format, ...);
extern uint64_t sysctl_get_time_us(void);
/* 工程中函数 */
extern void set_W_LED(int state);

void qrcode_convert_order(qrcode_image_t *img);
void qrcode_convert_to_gray(qrcode_image_t *img, uint8_t *gray);

enum enum_qrcode_res qrcode_scan(qrcode_scan_t *scan, uint8_t convert);

#endif /* __QRCODE_H */
