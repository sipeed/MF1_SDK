#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"
#include "img_op.h"

#if CONFIG_LCD_TYPE_ST7789
#include "lcd_st7789.h"
#elif CONFIG_LCD_TYPE_SSD1963
#include "lcd_ssd1963.h"
#elif CONFIG_LCD_TYPE_SIPEED
#include "lcd_sipeed.h"
#endif

/* clang-format off */
#define PASS_SAVE_PIC       (0)
/* clang-format on */

#if PASS_SAVE_PIC

#include "jpeg_compress.h"

#define JPEG_LEN (15 * 1024)
uint8_t jpeg_buf[JPEG_LEN];
uint8_t jpeg_tmp[IMG_W * IMG_H * 2];
#endif

extern void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* uart send all fea is compressed and base64 encoded, you can use base64 decode,and decompress it */
void decompress_feature(float feature[FEATURE_DIMENSION], int8_t compress_feature[FEATURE_DIMENSION])
{
    for(uint16_t i = 0; i < FEATURE_DIMENSION; i++)
    {
        feature[i] = (float)((float)compress_feature[i] / 512.0 /* /256 * 0.5 */);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//所有的操作都在dvo输出的图像中进行操作，然后再转换。
void lcd_draw_edge(face_obj_t *obj, uint32_t color)
{
    uint16_t color16 = rgb8882rgb565(color);
    uint32_t *gram = (uint32_t *)&display_image;
    uint32_t data = ((uint32_t)color16 << 16) | (uint32_t)color16;
    uint32_t *addr1, *addr2, *addr3, *addr4, x1, y1, x2, y2;

    x1 = obj->x1;
    y1 = obj->y1;
    x2 = obj->x2;
    y2 = obj->y2;

    if(x1 <= 0)
        x1 = 1;
    if(x2 >= IMG_W - 1)
        x2 = IMG_W - 2;
    if(y1 <= 0)
        y1 = 1;
    if(y2 >= IMG_H - 1)
        y2 = IMG_H - 2;

    addr1 = gram + (IMG_W * y1 + x1) / 2;
    addr2 = gram + (IMG_W * y1 + x2 - 8) / 2;
    addr3 = gram + (IMG_W * (y2 - 1) + x1) / 2;
    addr4 = gram + (IMG_W * (y2 - 1) + x2 - 8) / 2;
    for(uint32_t i = 0; i < 4; i++)
    {
        *addr1 = data;
        *(addr1 + IMG_W / 2) = data;
        *addr2 = data;
        *(addr2 + IMG_W / 2) = data;
        *addr3 = data;
        *(addr3 + IMG_W / 2) = data;
        *addr4 = data;
        *(addr4 + IMG_W / 2) = data;
        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }
    addr1 = gram + (IMG_W * y1 + x1) / 2;
    addr2 = gram + (IMG_W * y1 + x2 - 2) / 2;
    addr3 = gram + (IMG_W * (y2 - 8) + x1) / 2;
    addr4 = gram + (IMG_W * (y2 - 8) + x2 - 2) / 2;
    for(uint32_t i = 0; i < 8; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;
        addr1 += IMG_W / 2;
        addr2 += IMG_W / 2;
        addr3 += IMG_W / 2;
        addr4 += IMG_W / 2;
    }
    return;
}

static uint16_t Fast_AlphaBlender(uint32_t x, uint32_t y, uint32_t Alpha)
{
    x = (x | (x << 16)) & 0x7E0F81F;
    y = (y | (y << 16)) & 0x7E0F81F;
    uint32_t result = ((((x - y) * Alpha) >> 5) + y) & 0x7E0F81F;
    return (uint16_t)((result & 0xFFFF) | (result >> 16));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if CONFIG_LCD_TYPE_ST7789

//320x240 not support now

void display_lcd_img_addr(uint32_t pic_addr)
{
#if LCD_240240
    my_w25qxx_read_data(pic_addr, lcd_image, IMG_LCD_SIZE, W25QXX_STANDARD);
    lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image);
#endif
    return;
}

#if LCD_240240
static void display_fit_lcd(void)
{
    int x0, x1, x, y;
    uint16_t r, g, b;
    uint16_t *src = (uint16_t *)display_image;
    uint16_t *dest = (uint16_t *)lcd_image;
    x0 = (IMG_W - LCD_W) / 2;
    x1 = x0 + LCD_W;
    for(y = 0; y < LCD_H; y++)
    {
        memcpy(lcd_image + y * LCD_W * 2, display_image + (y * IMG_W + x0) * 2, LCD_W * 2);
    }
    return;
}
#endif

//aplha is stack up picture's diaphaneity
//aplha指的是叠加的图片的透明度
void display_fit_lcd_with_alpha(uint8_t *pic_addr, uint8_t *out_img, uint8_t alpha)
{
    int x0, x1, x, y;
    my_w25qxx_read_data(pic_addr, out_img, IMG_LCD_SIZE, W25QXX_STANDARD);
    uint16_t *s0 = (uint16_t *)display_image;
    uint16_t *s1 = (uint16_t *)out_img;
    x0 = (IMG_W - LCD_W) / 2;
    x1 = x0 + LCD_W;

    for(y = 0; y < LCD_H; y++)
    {
        for(x = 0; x < LCD_W; x++)
        {
            s1[y * LCD_W + x] = Fast_AlphaBlender((uint32_t)s0[y * IMG_W + x + x0], (uint32_t)s1[y * LCD_W + x], (uint32_t)alpha / 8);
        }
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_refresh_cb(void)
{
#if LCD_240240
    display_fit_lcd();
    lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image);
#else
    lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)display_image);
#endif
    return;
}

void lcd_draw_pass(void)
{
    display_fit_lcd_with_alpha(IMG_FACE_PASS_ADDR, lcd_image, 128);
    lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image); //7.5ms
    face_lib_draw_flag = 1;
    msleep(500); //display
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void detected_face_cb(face_recognition_ret_t *face)
{
    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    if(g_key_press)
    {
        get_date_time();
    }

    for(uint32_t i = 0; i < face_cnt; i++)
    {
        face_obj_t *face_info = &face->result->face_obj_info.obj[i];
        if(face_info->pass == false)
        {
            lcd_draw_edge(face_info, 0xff0000);
        }

        if(g_key_press)
        {
            /* only one person then record face ???? */
            if(flash_save_face_info(NULL, face_info->feature, NULL, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
            {
                printk("Feature Full\n");
                break;
            }
            display_fit_lcd_with_alpha(IMG_RECORD_FACE_ADDR, lcd_image, 128);
            lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image); //7.5ms
            face_lib_draw_flag = 1;
            set_RGB_LED(GLED);
            msleep(500);
            set_RGB_LED(0);
        }
    }

    if(g_key_press)
    {
        g_key_press = 0;
    }
}

/* ir check faild */
void fake_face_cb(face_recognition_ret_t *face)
{
    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    face_obj_t *face_info = NULL;

    for(uint32_t i = 0; i < face_cnt; i++)
    {
        face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
        lcd_draw_edge(face_info, 0x0000ff); //BLUE
    }
}

/* ir check pass or auto pass */
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check)
{
    uint32_t face_cnt = 0;
    face_obj_t *face_info = NULL;
    uint64_t tim = sysctl_get_time_us();

#if PASS_SAVE_PIC
    jpeg_encode_t jpeg_src, jpeg_out;

    memcpy(jpeg_tmp, display_image, IMG_W * IMG_H * 2); //如果不拷贝，会影响显示的内容
    reverse_u32pixel(jpeg_tmp, 320 * 240 / 2);

    jpeg_src.w = 320;
    jpeg_src.h = 240;
    jpeg_src.bpp = 2;
    jpeg_src.data = jpeg_tmp;

    jpeg_out.w = jpeg_src.w;
    jpeg_out.h = jpeg_src.h;
    jpeg_out.bpp = JPEG_LEN;
    jpeg_out.data = jpeg_buf;

    //将摄像头输出图像压缩成Jpeg,压缩后图像大小为jpeg_out.bpp
    if(jpeg_compress(&jpeg_src, &jpeg_out, 80, 0) == 0)
    {
        printf("w:%d\th:%d\tbpp:%d\r\n", jpeg_out.w, jpeg_out.h, jpeg_out.bpp);
    } else
    {
        printf("jpeg encode failed\r\n");
    }
    printk("jpeg compress: %ld us\r\n", sysctl_get_time_us() - tim);
    tim = sysctl_get_time_us();
#endif

    if(ir_check)
    {
        face_cnt = face->result->face_compare_info.result_number;

        for(uint32_t i = 0; i < face_cnt; i++)
        {
            face_info = (face_obj_t *)(face->result->face_compare_info.obj[i]);
            face_pass_callback(face_info, face_cnt, i, &tim);
            lcd_draw_edge(face_info, 0x00ff00); //GREEN
        }
    } else
    {
        face_cnt = face->result->face_obj_info.obj_number;

        for(uint32_t i = 0; i < face_cnt; i++)
        {
            if((g_board_cfg.auto_out_feature & 1) == 1)
            {
                face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
                face_pass_callback(face_info, face_cnt, i, &tim);
                lcd_draw_edge(face_info, 0xffffff); //WHITE
            } else
            {
                printk("unknow\r\n");
            }
        }
    }
}

#elif CONFIG_LCD_TYPE_SSD1963
void lcd_convert_cam_to_lcd(void)
{
    return;
}

void lcd_draw_picture_cb(void)
{
    return;
}
void record_face(face_recognition_ret_t *ret)
{
    return;
}

void lcd_draw_pass(void)
{
    return;
}

#else /* CONFIG_LCD_TYPE_SIPEED */

static uint8_t line_buf[IMG_W * 2];

void display_fit_lcd_with_alpha_v1(uint8_t *pic_flash_addr, uint8_t *in_img, uint8_t *out_img, uint8_t alpha)
{
    //alpha pic lines to in_img
    int x0, x1, x, y;

    uint16_t *s0 = line_buf;
    uint16_t *s1 = in_img;

    memset(line_buf, 0xff, 2 * (IMG_W - DAT_W) / 2);
    memset(line_buf + 2 * (IMG_W - DAT_W) / 2 + DAT_W * 2, 0xff, 2 * (IMG_W - DAT_W) / 2);

    for(y = 0; y < IMG_H; y++)
    {
        my_w25qxx_read_data(pic_flash_addr + y * DAT_W * 2, line_buf + 2 * (IMG_W - DAT_W) / 2, DAT_W * 2, W25QXX_STANDARD);
        for(x = 0; x < IMG_W; x++)
        {
            s1[y * IMG_W + x] = Fast_AlphaBlender((uint32_t)s0[x], (uint32_t)s1[y * IMG_W + x], (uint32_t)alpha / 8);
        }
    }
    lcd_covert_cam_order((uint32_t *)in_img, (IMG_W * IMG_H / 2));

    while(dis_flag)
        ;

    copy_image_cma_to_lcd(in_img, out_img);

    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_draw_pass(void)
{
    printk("face pass\r\n");
    display_fit_lcd_with_alpha_v1(IMG_FACE_PASS_ADDR, display_image, lcd_image, 128);
    face_lib_draw_flag = 1;
    set_RGB_LED(GLED);
    msleep(500);
    set_RGB_LED(0);
    return;
}

void lcd_refresh_cb(void)
{
    lcd_covert_cam_order((uint32_t *)display_image, (IMG_W * IMG_H / 2));
    while(dis_flag)
        ;
    copy_image_cma_to_lcd(display_image, lcd_image);
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void detected_face_cb(face_recognition_ret_t *face)
{
    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    if(g_key_press)
    {
        get_date_time();
    }

    for(uint32_t i = 0; i < face_cnt; i++)
    {
        face_obj_t *face_info = &face->result->face_obj_info.obj[i];
        if(face_info->pass == false)
        {
            lcd_draw_edge(face_info, 0xff0000);
        }

        if(g_key_press)
        {
            /* only one person then record face ???? */
            if(flash_save_face_info(NULL, face_info->feature, NULL, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
            {
                printk("Feature Full\n");
                break;
            }
            display_fit_lcd_with_alpha_v1(IMG_RECORD_FACE_ADDR, display_image, lcd_image, 128);
            face_lib_draw_flag = 1;
            set_RGB_LED(GLED);
            msleep(500);
            set_RGB_LED(0);
        }
    }

    if(g_key_press)
    {
        g_key_press = 0;
    }
}

void fake_face_cb(face_recognition_ret_t *face)
{
    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    face_obj_t *face_info = NULL;

    for(uint32_t i = 0; i < face_cnt; i++)
    {
        face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
        lcd_draw_edge(face_info, 0x0000ff); //BLUE
    }
}

void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check)
{
    uint32_t face_cnt = 0;
    face_obj_t *face_info = NULL;
    uint64_t tim = sysctl_get_time_us();

    if(ir_check)
    {
        face_cnt = face->result->face_compare_info.result_number;

        for(uint32_t i = 0; i < face_cnt; i++)
        {
            face_info = (face_obj_t *)(face->result->face_compare_info.obj[i]);
            face_pass_callback(face_info, face_cnt, i, &tim);
            lcd_draw_edge(face_info, 0x00ff00); //GREEN
        }
    } else
    {
        face_cnt = face->result->face_obj_info.obj_number;

        for(uint32_t i = 0; i < face_cnt; i++)
        {
            if((g_board_cfg.auto_out_feature & 1) == 1)
            {
                face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
                face_pass_callback(face_info, face_cnt, i, &tim);
                lcd_draw_edge(face_info, 0xffffff); //WHITE
            } else
            {
                printk("unknow\r\n");
            }
        }
    }
}
#endif
