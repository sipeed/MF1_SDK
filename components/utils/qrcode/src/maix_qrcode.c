#include "maix_qrcode.h"

#include "global_config.h"

#if CONFIG_QRCODE_TYPE_QUIRC
#include "quirc.h"
#endif /* CONFIG_QRCODE_TYPE_QUIRC */

#if CONFIG_QRCODE_TYPE_ZBAR
// #include "zbar.h"
#endif /* CONFIG_QRCODE_TYPE_ZBAR */

///////////////////////////////////////////////////////////////////////////////
void qrcode_convert_order(qrcode_image_t *img)
{
    uint16_t t[2], *ptr = (uint16_t *)(img->data);

    for (uint32_t i = 0; i < (img->w * img->h); i += 2)
    {
        t[0] = *(ptr + 1);
        t[1] = *(ptr);
        *(ptr) = SWAP_16(t[0]);
        *(ptr + 1) = SWAP_16(t[1]);
        ptr += 2;
    }
    return;
}

void qrcode_convert_to_gray(qrcode_image_t *img, uint8_t *gray)
{
    for (int y = 0, yy = img->h; y < yy; y++)
    {
        uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
        for (int x = 0, xx = img->w; x < xx; x++)
        {
            *(gray++) = IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, img->w - x) & 0xff;
            // *(grayscale_image++) = COLOR_RGB565_TO_GRAYSCALE(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, img->w - x));
        }
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////
/* 0: 扫码成功
   1: 扫码失败
   2: 扫码超时
   3: 二维码内容太大
   4: 未知异常
   */
enum enum_qrcode_res qrcode_scan(qrcode_scan_t *scan, uint8_t convert)
{
    enum enum_qrcode_res ret;
    qrcode_image_t img;

#if !CONFIG_ALWAYS_SCAN_QRCODES
    if ((sysctl_get_time_us() - (scan->start_time_us)) > (scan->scan_time_out_s * 1000 * 1000))
    {
        return QRCODE_TIMEOUT;
    }
#endif /* CONFIG_ALWAYS_SCAN_QRCODES */

    if (scan->img_data == NULL)
    {
        return QRCODE_ERROR;
    }

    memset(&img, 0, sizeof(img));
    img.w = scan->img_w; /* 320 */
    img.h = scan->img_h; /* 240 */
    img.data = (uint8_t *)(scan->img_data);

    scan->qrcode_num = 0;

#if CONFIG_QRCODE_TYPE_QUIRC
    qrcode_result_t qrcode;
    if (find_qrcodes(&qrcode, &img, scan, convert) != 0)
    {
        // if (/* qrcode.version <= 6 && */ qrcode.payload_len >= QUIRC_MAX_PAYLOAD)
        // {
        //     printk("qrcode content too big than buffer\r\n");
        //     return QRCODE_TOOBIG;
        // }
        // else
        {
            memcpy(scan->qrcode, qrcode.payload, qrcode.payload_len);
            scan->qrcode[qrcode.payload_len] = 0;
            ret = QRCODE_SUCC;
        }
    }
#elif CONFIG_QRCODE_TYPE_ZBAR
    extern uint8_t find_qrcodes(qrcode_image_t * img, qrcode_scan_t * scan, uint8_t convert);
    if (find_qrcodes(&img, scan, convert) != 0)
    {
        ret = QRCODE_SUCC;
    }
#else
#error "Must choose one qrcode engine"
#endif
    else
    {
        ret = QRCODE_NONE;
    }

#if CONFIG_ALWAYS_SCAN_QRCODES
    /* 有二维码就开led，没有就关闭 */
    set_W_LED(scan->qrcode_num ? 1 : 0);
#endif

    return ret;
}
