#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"

#include "image_op.h"
#include "lcd_dis.h"

#include "lcd.h"

#include "global_config.h"

extern void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);
///////////////////////////////////////////////////////////////////////////////
uint8_t delay_flag = 0;

static uint8_t *pDisImage = NULL;
static uint16_t DisImage_W, DisImage_H;
static uint16_t DisLcd_W, DisLcd_H;
static uint16_t DisX_Off, DisY_Off;
static uint16_t DisImageX_Off, DisImageY_Off;

#define LCD_OFT 40

void face_cb_init(void)
{
#if CONFIG_LCD_VERTICAL
    pDisImage = display_image_ver;

    DisImage_W = CONFIG_CAMERA_RESOLUTION_HEIGHT;
    DisImage_H = CONFIG_CAMERA_RESOLUTION_WIDTH;

    DisLcd_W = CONFIG_LCD_HEIGHT;
    DisLcd_H = CONFIG_LCD_WIDTH;

    DisX_Off = 0;

    DisImageX_Off = 0;
    DisImageY_Off = 40;
#else
    pDisImage = display_image;

    DisImage_W = CONFIG_CAMERA_RESOLUTION_WIDTH;
    DisImage_H = CONFIG_CAMERA_RESOLUTION_HEIGHT;

    DisLcd_W = CONFIG_LCD_WIDTH;
    DisLcd_H = CONFIG_LCD_HEIGHT;

    DisX_Off = LCD_OFT;

    DisImageX_Off = LCD_OFT;
    DisImageY_Off = 0;
#endif

    DisY_Off = DisLcd_H - 16;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if (CONFIG_LCD_WIDTH == 240)
static void convert_320x240_to_240x240(uint8_t *img_320, uint16_t x_offset)
{
    for (uint8_t i = 0; i < 240; i++)
    {
        memcpy(img_320 + i * 240 * 2, img_320 + i * 320 * 2 + x_offset * 2, 240 * 2);
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void lcd_convert_cb(void)
{
#if CONFIG_LCD_VERTICAL
    //5300us @ 400M
    convert_rgb565_order((uint16_t *)display_image, CONFIG_CAMERA_RESOLUTION_WIDTH, CONFIG_CAMERA_RESOLUTION_HEIGHT);
    image_rgb565_roate_right90((uint16_t *)display_image_ver, (uint16_t *)display_image, CONFIG_CAMERA_RESOLUTION_WIDTH, CONFIG_CAMERA_RESOLUTION_HEIGHT);
    convert_rgb565_order((uint16_t *)display_image_ver, CONFIG_CAMERA_RESOLUTION_HEIGHT, CONFIG_CAMERA_RESOLUTION_WIDTH);
#endif

    /* 在这里遍历lcd显示链表*/
    if (lcd_dis_list->len)
    {
        list_node_t *node = NULL;
        list_iterator_t *lcd_dis_list_iterator = list_iterator_new(lcd_dis_list, LIST_HEAD);

        if (lcd_dis_list_iterator)
        {
            while ((node = list_iterator_next(lcd_dis_list_iterator)))
            {
                lcd_dis_t *lcd_dis = (lcd_dis_t *)node->val;
                lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
                if (lcd_dis->auto_del)
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
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void lcd_refresh_cb(void)
{
#if (CONFIG_LCD_WIDTH == 240)
    convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

    if (delay_flag)
    {
        delay_flag = 0;
        msleep(300);
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void lcd_draw_pass(void)
{
    lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, IMG_FACE_PASS_ADDR, 128, DisImageX_Off, DisImageY_Off, 240, 240);

    if (lcd_dis)
    {
        lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
        lcd_dis_list_del_by_id(255);
    }
    else
    {
        image_rgb565_draw_string(pDisImage, "Face Pass...", DisX_Off, DisY_Off, WHITE, NULL, DisImage_W);
    }

#if (CONFIG_LCD_WIDTH == 240)
    convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

    face_lib_draw_flag = 1;
    set_RGB_LED(RLED);
    msleep(500); //this delay can modify
    set_RGB_LED(0);
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* ir check faild */
void fake_face_cb(face_recognition_ret_t *face)
{
    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    face_obj_t *face_info = NULL;

    for (uint32_t i = 0; i < face_cnt; i++)
    {
        face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
        image_rgb565_draw_edge(pDisImage, face_info->x1, face_info->y1, face_info->x2, face_info->y2, BLUE, DisImage_W, DisImage_H);
    }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void detected_face_cb(face_recognition_ret_t *face)
{
    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    for (uint32_t i = 0; i < face_cnt; i++)
    {
        face_obj_t *face_info = &(face->result->face_obj_info.obj[i]);
        if (face_info->pass == false)
        {
            printk("false face\r\n");
            image_rgb565_draw_edge(pDisImage, face_info->x1, face_info->y1, face_info->x2, face_info->y2, RED, DisImage_W, DisImage_H);

            uint16_t bg = 0x0;
            char str[20];
            sprintf(str, "%.3f", face_info->score);

            image_rgb565_draw_string(pDisImage, str, DisX_Off, DisY_Off, WHITE, &bg, DisImage_W);
        }

        if (g_key_press)
        {
            /* only one person then record face ???? */
            if (face_info->feature)
            {
                face_fea_t face_fea;

#if CONFIG_CAMERA_GC0328_DUAL
                face_fea.stat = 1;
                memcpy(&(face_fea.fea_ir), face_info->feature, 196);
                memset(&(face_fea.fea_rgb), 0, 196);
#else
                face_fea.stat = 0;
                memcpy(&(face_fea.fea_rgb), face_info->feature, 196);
                memset(&(face_fea.fea_ir), 0, 196);
#endif

                printf("#######save face\r\n");
                if (flash_save_face_info(&face_fea, NULL, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
                {
                    printk("Feature Full\n");
                    break;
                }

                lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, IMG_RECORD_FACE_ADDR, 128, DisImageX_Off, DisImageY_Off, 240, 240);
                if (lcd_dis)
                {
                    lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
                    lcd_dis_list_del_by_id(255);
                }
                else
                {
                    image_rgb565_draw_string(pDisImage, "Recording Face...", DisX_Off, DisY_Off, WHITE, NULL, DisImage_W);
                }

#if (CONFIG_LCD_WIDTH == 240)
                convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

                lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

                face_lib_draw_flag = 1;
                set_RGB_LED(GLED);
                msleep(500); //this delay can modify
                set_RGB_LED(0);
            }
        }
    }

    if (g_key_press)
    {
        g_key_press = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if CONFIG_CAMERA_GC0328_DUAL
/* ir check pass or auto pass */
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check)
{
    uint32_t face_cnt = 0;
    face_obj_t *face_info = NULL;
    uint64_t tim = sysctl_get_time_us();

    if (ir_check)
    {
        face_cnt = face->result->face_compare_info.result_number;

        for (uint32_t i = 0; i < face_cnt; i++)
        {
            face_info = (face_obj_t *)(face->result->face_compare_info.obj[i]);
            image_rgb565_draw_edge(pDisImage, face_info->x1, face_info->y1, face_info->x2, face_info->y2, GREEN, DisImage_W, DisImage_H);
            face_pass_callback(face_info, face_cnt, i, &tim);
        }
    }
    else
    {
        face_cnt = face->result->face_obj_info.obj_number;

        for (uint32_t i = 0; i < face_cnt; i++)
        {
            if ((g_board_cfg.auto_out_feature & 1) == 1)
            {
                face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
                face_pass_callback(face_info, face_cnt, i, &tim);
                image_rgb565_draw_edge(pDisImage, face_info->x1, face_info->y1, face_info->x2, face_info->y2, WHITE, DisImage_W, DisImage_H);
            }
            else
            {
                printk("unknow\r\n");
            }
        }
    }
}
#else
void pass_face_cb(face_recognition_ret_t *face, uint8_t ir_check)
{
    uint64_t v_tick;
    uint32_t face_num = 0;
    face_obj_t *face_info = NULL;

    v_tick = sysctl_get_time_us();

    if (g_board_cfg.auto_out_feature == 1)
    {
        //不与数据库的人脸数据进行对比，直接输出识别到的
        face_num = face->result->face_obj_info.obj_number;

        for (uint32_t i = 0; i < face_num; i++)
        {
            face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
            face_pass_callback(face_info, face_num, i, &v_tick);
            image_rgb565_draw_edge(pDisImage, face_info->x1, face_info->y1, face_info->x2, face_info->y2, YELLOW, DisImage_W, DisImage_H);
        }
    }
    else
    {
        //需要进行对比
        face_num = face->result->face_compare_info.result_number;

        for (uint32_t i = 0; i < face_num; i++)
        {
            face_info = (face_obj_t *)(face->result->face_compare_info.obj[i]);
            face_pass_callback(face_info, face_num, i, &v_tick);
            if (face_info->pass)
            {
                image_rgb565_draw_edge(pDisImage, face_info->x1, face_info->y1, face_info->x2, face_info->y2, GREEN, DisImage_W, DisImage_H);
            }
        }
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
