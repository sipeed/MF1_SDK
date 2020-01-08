#include "zbar.h"

#include "maix_qrcode.h"

uint8_t buf[320 * 240];

// int Zbar_Test(uint16_t *imgbuf, int width, int height)

/* return: 0:noqrcode other have qrcode */
uint8_t find_qrcodes(qrcode_image_t *img, qrcode_scan_t *scan, uint8_t convert)
{
    /* 先申请内存，如果失败则直接返回 */
    uint8_t *gray = malloc(img->w * img->h);
    if (!gray)
    {
        printk("malloc failed %s:%d\r\n", __func__, __LINE__);
        return 0;
    }

    /* 转换图像的字节序 */
    qrcode_convert_order(img);
    /* 将图像转换为灰度 */
    qrcode_convert_to_gray(img, gray);

    /* 开始调用Zbar进行扫码 */
    /* create a reader */
    zbar_image_scanner_t *scanner = zbar_image_scanner_create();
    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
    /* wrap image data */
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, *(int *)"Y800");
    zbar_image_set_size(image, img->w, img->h);
    zbar_image_set_data(image, gray, img->w * img->h, NULL); //zbar_image_free_data
    /* scan the image for barcodes */
    int qrcode_num = zbar_scan_image(scanner, image);
    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);

    /* 如果有多个二维码，会把最后一个结果返回 */
    for (; symbol; symbol = zbar_symbol_next(symbol))
    {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        size_t len = strlen(data);

        printf("\r\ndecoded symbol: %s, content: \"%s\", len = %ld\r\n", zbar_get_symbol_name(typ), data, len);

        /* 拷贝到结果 */
        if (len < QUIRC_MAX_PAYLOAD)
        {
            memcpy(scan->qrcode, data, len);
            scan->qrcode[len] = 0;
        }
        else
        {
            /* 这里溢出了,就不进行拷贝 */
            qrcode_num = 0;
        }
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);
    qrcode_convert_order(img);

    if (gray)
        free(gray);

    scan->qrcode_num = (uint8_t)qrcode_num;

    return qrcode_num;
}
