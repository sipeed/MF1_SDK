#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"

#include "image_op.h"
#include "lcd_dis.h"

#include "lcd.h"

/* clang-format off */
#define PASS_SAVE_PIC       (0)
/* clang-format on */

#if PASS_SAVE_PIC

#include "jpeg_compress.h"

#define JPEG_LEN (15 * 1024)
uint8_t jpeg_buf[JPEG_LEN];
uint8_t jpeg_tmp[IMG_W * IMG_H * 2];
#endif

uint8_t delay_flag = 0;

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

static uint8_t *pDisImage = NULL;
static uint16_t DisImage_W, DisImage_H;
static uint16_t DisLcd_W, DisLcd_H;
static uint16_t DisX_Off, DisY_Off;

void face_cb_init(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if CONFIG_LCD_TYPE_ST7789

#if LCD_240240
static void convert_320x240_to_240x240(uint8_t *img_320, uint16_t x_offset)
{
    for(uint8_t i = 0; i < 240; i++)
    {
        memcpy(img_320 + i * 240 * 2, img_320 + i * 320 * 2 + x_offset * 2, 240 * 2);
    }
}
#endif

void lcd_convert_cb(void)
{
#if CONFIG_DETECT_VERTICAL
    //5300us @ 400M
    convert_rgb565_order((uint16_t *)display_image, LCD_W, LCD_H);
    image_rgb565_roate_right90((uint16_t *)display_image_ver, (uint16_t *)display_image, LCD_W, LCD_H);
    convert_rgb565_order((uint16_t *)display_image_ver, LCD_H, LCD_W);
#endif

    /* 在这里遍历lcd显示链表*/
    if(lcd_dis_list->len)
    {
        list_node_t *node = NULL;
        list_iterator_t *lcd_dis_list_iterator = list_iterator_new(lcd_dis_list, LIST_HEAD);

        if(lcd_dis_list_iterator)
        {
            while((node = list_iterator_next(lcd_dis_list_iterator)))
            {
                lcd_dis_t *lcd_dis = (lcd_dis_t *)node->val;
                lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
                if(lcd_dis->auto_del)
                {
                    lcd_dis_list_free(lcd_dis);
                    list_remove(lcd_dis_list, node);
                }
            }
            list_iterator_destroy(lcd_dis_list_iterator);
            lcd_dis_list_iterator = NULL;
        }
    }
}

void display_lcd_img_addr(uint32_t pic_addr)
{
    lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, pic_addr, 128, DisX_Off, DisY_Off, 240, 240);
    if(lcd_dis)
    {
        lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
        lcd_dis_list_del_by_id(255);
    } else
    {
        image_rgb565_draw_string(pDisImage, "MEMRORY FULL", 0, LCD_W - 16, WHITE, NULL, DisImage_W);
    }

#if LCD_240240
    convert_320x240_to_240x240(display_image, DisX_Off);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_refresh_cb(void)
{
#if LCD_240240
    convert_320x240_to_240x240(display_image, DisX_Off);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

    if(delay_flag)
    {
        delay_flag = 0;
        msleep(300);
    }
    return;
}

void lcd_draw_pass(void)
{
    lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, IMG_FACE_PASS_ADDR, 128, DisX_Off, DisY_Off, 240, 240);
    if(lcd_dis)
    {
        lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
        lcd_dis_list_del_by_id(255);
    } else
    {
        image_rgb565_draw_string(pDisImage, "Face Pass...", 0, LCD_W - 16, WHITE, NULL, DisImage_W);
    }

#if LCD_240240
    convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

    face_lib_draw_flag = 1;
    set_RGB_LED(RLED);
    msleep(500); //this delay can modify
    set_RGB_LED(0);
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
        face_obj_t *face_info = &(face->result->face_obj_info.obj[i]);
        if(face_info->pass == false)
        {
            printk("false face\r\n");

            image_rgb565_draw_edge(pDisImage, face_info, RED, DisImage_W, DisImage_H);

            uint16_t bg = 0x0;
            char str[20];
            sprintf(str, "%.3f", face_info->score);
            image_rgb565_draw_string(pDisImage, str, 0, LCD_W - 16, WHITE, &bg, DisImage_W);
        }

        if(g_key_press)
        {
            /* only one person then record face ???? */
            if(face_info->feature)
            {
                face_fea_t face_fea;
#if CONFI_SINGLE_CAMERA
                face_fea.stat = 0;
                memcpy(&(face_fea.fea_rgb), face_info->feature, 196);
                memset(&(face_fea.fea_ir), 0, 196);
#else
                face_fea.stat = 1;
                memcpy(&(face_fea.fea_ir), face_info->feature, 196);
                memset(&(face_fea.fea_rgb), 0, 196);
#endif

                printf("#######save face\r\n");
                if(flash_save_face_info(&face_fea, NULL, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
                {
                    printk("Feature Full\n");
                    break;
                }

#if CONFIG_DETECT_VERTICAL
                lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, IMG_RECORD_FACE_ADDR, 128, 0, 40, 240, 240);
#else
                lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, IMG_RECORD_FACE_ADDR, 128, LCD_OFT, 0, 240, 240);
#endif

                if(lcd_dis)
                {
#if CONFIG_DETECT_VERTICAL
                    lcd_dis_list_display(display_image_ver, IMG_H, IMG_W, lcd_dis);
#else
                    lcd_dis_list_display(display_image, IMG_W, IMG_H, lcd_dis);
#endif

                    lcd_dis_list_del_by_id(255);
                } else
                {
#if CONFIG_DETECT_VERTICAL
                    image_rgb565_draw_string(display_image_ver, "Recording Face...", 0, LCD_W - 16, WHITE, NULL, IMG_H);
#else
                    image_rgb565_draw_string(display_image, "Recording Face...", LCD_OFT, LCD_H - 16, WHITE, NULL, IMG_W);
#endif
                }
#if LCD_240240
                convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

#if CONFIG_DETECT_VERTICAL
                lcd_draw_picture(0, 0, LCD_H, LCD_W, (uint32_t *)display_image_ver);
#else
                lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)display_image);
#endif

                face_lib_draw_flag = 1;
                set_RGB_LED(GLED);
                msleep(500); //this delay can modify
                set_RGB_LED(0);
            }
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
#if CONFIG_DETECT_VERTICAL
        image_rgb565_draw_edge(display_image_ver, face_info, BLUE, IMG_H, IMG_W);
#else
        image_rgb565_draw_edge(display_image, face_info, BLUE, IMG_W, IMG_H);
#endif
    }
}

#if CONFI_SINGLE_CAMERA
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check)
{
    uint64_t v_tick;
    uint32_t face_num = 0;
    face_obj_t *face_obj = NULL;

    v_tick = sysctl_get_time_us();

    if(g_board_cfg.auto_out_feature == 1)
    {
        //不与数据库的人脸数据进行对比，直接输出识别到的
        face_num = face->result->face_obj_info.obj_number;

        for(uint32_t i = 0; i < face_num; i++)
        {
            face_obj = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
            face_pass_callback(face_obj, face_num, i, &v_tick);
#if DETECT_VERTICAL
            image_rgb565_draw_edge(display_image_ver, face_obj, YELLOW, LCD_H, LCD_W);
#else
            image_rgb565_draw_edge(display_image, face_obj, YELLOW, LCD_W, LCD_H);
#endif
        }
    } else
    {
        //需要进行对比
        face_num = face->result->face_compare_info.result_number;

        for(uint32_t i = 0; i < face_num; i++)
        {
            face_obj = (face_obj_t *)(face->result->face_compare_info.obj[i]);
            face_pass_callback(face_obj, face_num, i, &v_tick);
            if(face_obj->pass)
            {
#if DETECT_VERTICAL
                image_rgb565_draw_edge(display_image_ver, face_obj, GREEN, LCD_H, LCD_W);
#else
                image_rgb565_draw_edge(display_image, face_obj, GREEN, LCD_W, LCD_H);
#endif
            }
        }
    }
}
#else
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
#if CONFIG_DETECT_VERTICAL
            image_rgb565_draw_edge(display_image_ver, face_info, GREEN, 240, 320);
#else
            image_rgb565_draw_edge(display_image, face_info, GREEN, 320, 240);
#endif

            char str[20];
            uint16_t bg = 0x0;
            sprintf(str, "%d:%.3f", face_info->index, face_info->score);

#if CONFIG_DETECT_VERTICAL
            image_rgb565_draw_string(display_image_ver, str, 0, LCD_W - 16, WHITE, &bg, 240);
#else
            image_rgb565_draw_string(display_image, str, LCD_OFT, LCD_H - 16, WHITE, &bg, 320);
#endif

            face_pass_callback(face_info, face_cnt, i, &tim);
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
#if CONFIG_DETECT_VERTICAL
                image_rgb565_draw_edge(display_image_ver, face_info, WHITE, 240, 320);
#else
                image_rgb565_draw_edge(display_image, face_info, WHITE, 320, 240);
#endif
            } else
            {
                printk("unknow\r\n");
            }
        }
    }
}
#endif

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
