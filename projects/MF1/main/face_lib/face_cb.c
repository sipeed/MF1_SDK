#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"

#include "image_op.h"
#include "lcd_dis.h"

#include "lcd.h"

#include "global_config.h"

#if CONFIG_ENABLE_OUTPUT_JPEG
#include "core1.h"
#endif /* CONFIG_ENABLE_OUTPUT_JPEG */

extern void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);
///////////////////////////////////////////////////////////////////////////////
/* protocol record face */
static proto_record_face_cfg_t record_cfg;

static uint8_t proto_record_flag = 0;
static uint64_t proto_record_start_time = 0;

///////////////////////////////////////////////////////////////////////////////
uint8_t delay_flag = 0;
uint8_t lcd_bl_stat = 1; /* 1:on, 0:off */

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
void convert_320x240_to_240x240(uint8_t *img_320, uint16_t x_offset)
{
    for (uint8_t i = 0; i < 240; i++)
    {
        memcpy(img_320 + i * 240 * 2, img_320 + i * 320 * 2 + x_offset * 2, 240 * 2);
    }
}
#endif

void lcd_display_image_alpha(uint32_t pic_addr, uint32_t alpha)
{
    lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(CONFIG_LIST_NODE_MAX_NUM - 1, 1, pic_addr, alpha, DisImageX_Off, DisImageY_Off, 240, 240);

    if (lcd_dis)
    {
        lcd_dis_list_display(pDisImage, DisImage_W, DisImage_H, lcd_dis);
        lcd_dis_list_del_by_id(CONFIG_LIST_NODE_MAX_NUM - 1);
    }
    else
    {
        image_rgb565_draw_string(pDisImage, "NO MEMRORY...", 16, DisX_Off, DisY_Off, WHITE, NULL, DisImage_W, DisImage_H);
    }

#if (CONFIG_LCD_WIDTH == 240)
    convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void lcd_convert_cb(void)
{
#if CONFIG_ENABLE_OUTPUT_JPEG
    send_jpeg_to_core1(display_image);
#endif /* CONFIG_ENABLE_OUTPUT_JPEG */

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
void lcd_close_bl_cb(void)
{
    if(lcd_bl_stat == 1)
    {
        lcd_bl_stat = 0;
        set_lcd_bl(0);
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void lcd_refresh_cb(void)
{
    if(lcd_bl_stat == 0)
    {
        lcd_bl_stat = 1;
        set_lcd_bl(1);
    }

#if (CONFIG_LCD_WIDTH == 240)
    convert_320x240_to_240x240(display_image, LCD_OFT);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);

    if (delay_flag)
    {
        delay_flag = 0;
        msleep(300);
    }

    if (proto_record_flag)
    {
        uint64_t tim = sysctl_get_time_us();
        tim = (tim - proto_record_start_time) / 1000 / 1000;

        if (tim > record_cfg.time_out_s)
        {
            record_cfg.send_ret(1, "timeout to record face", NULL);
            proto_record_flag = 0;
        }
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void lcd_draw_pass(void)
{
    lcd_display_image_alpha(IMG_FACE_PASS_ADDR, 80);

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
/* 依赖detected_face_cb */
void protocol_record_face(proto_record_face_cfg_t *cfg)
{
    proto_record_flag = 1;
    proto_record_start_time = sysctl_get_time_us();

    memcpy(&record_cfg, cfg, sizeof(proto_record_face_cfg_t));
    return;
}
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

            image_rgb565_draw_string(pDisImage, str, 16, DisX_Off, DisY_Off, WHITE, &bg, DisImage_W, DisImage_H);
        }
        printf("face prob:%.4f\r\n", face_info->prob);

        if (proto_record_flag)
        {
            uint64_t tim = sysctl_get_time_us();
            tim = (tim - proto_record_start_time) / 1000 / 1000;

            if (tim > record_cfg.time_out_s)
            {
                record_cfg.send_ret(1, "timeout to record face", NULL);
                proto_record_flag = 0;
                goto _exit;
            }

            if (judge_face_by_keypoint(&(face_info->key_point)) && (face_info->prob >= 0.92f))
            {
                printf("record face\r\n");

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
                if (flash_save_face_info(&face_fea, record_cfg.uid, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
                {
                    printk("Feature Full\n");
                    record_cfg.send_ret(1, "flash is full", NULL);
                    break;
                }

                record_cfg.send_ret(0, "record face success", record_cfg.uid);

                proto_record_flag = 0;

                lcd_display_image_alpha(IMG_RECORD_FACE_ADDR, 128);
                face_lib_draw_flag = 1;

                set_RGB_LED(GLED);
                msleep(500); //this delay can modify
                set_RGB_LED(0);
            }
        }
#if (CONFIG_SHORT_PRESS_FUNCTION_KEY_RECORD_FACE && (CONFIG_KEY_SHORT_QRCODE == 0))
        else
        {
            /* 按键录入的功能可以删除 */
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

                    lcd_display_image_alpha(IMG_RECORD_FACE_ADDR, 128);

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
#endif
    }
_exit:
    return;
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
            if (g_board_cfg.brd_soft_cfg.cfg.out_fea == 2)
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

    if (g_board_cfg.brd_soft_cfg.cfg.auto_out_fea)
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
//1、计算鼻子的坐标与双眼坐标中点的差值，大于10，认为没有正视
//2、计算鼻子和眼镜的 `y` 轴的差值，如果占整个人脸的比例小于xx，认为没有正视
uint8_t judge_face_by_keypoint(key_point_t *kp)
{
    uint8_t ret = 0;
    int32_t tmp = 0;
    uint32_t witdh, height;
    float percent = 0.0f;

    char str[32];

    witdh = kp->width;
    height = kp->height;

    ///////////////////////////////////////////////////////////////////////////////
    tmp = (kp->point[1].x - kp->point[0].x) / 2;
    tmp += kp->point[0].x;
    tmp = kp->point[2].x - tmp;

    if (tmp < 0)
        tmp = -tmp;

    printf("th1:%d\r\n", tmp);

    //阈值1
    if (tmp > 8)
    {
        printf("th2 not pass\r\n");
        ret++;
    }

    sprintf(str, "1:%d", tmp);
#if DETECT_VERTICAL
    image_rgb565_draw_string(display_image_ver, str, 16, 0, 16,
                             RED, NULL,
                             240, 320);
#else
    image_rgb565_draw_string(display_image, str, 16, 0, 16,
                             RED, NULL,
                             320, 240);
#endif

    ///////////////////////////////////////////////////////////////////////////////
    tmp = (kp->point[0].y + kp->point[1].y) / 2;
    tmp = kp->point[2].y - tmp;

    if (tmp < 0)
        tmp = -tmp;

    percent = (float)((float)tmp / (float)height);
    printf("th2:%.3f\r\n", percent);

    //阈值2
    if (percent < 0.14f) //0.14f
    {
        printf("th2 not pass\r\n");
        ret++;
    }

    sprintf(str, "2:%.4f", percent);
#if DETECT_VERTICAL
    image_rgb565_draw_string(display_image_ver, str, 16, 0, 32,
                             RED, NULL,
                             240, 320);
#else
    image_rgb565_draw_string(display_image, str, 16, 0, 32,
                             RED, NULL,
                             320, 240);
#endif

    ///////////////////////////////////////////////////////////////////////////////
    tmp = (kp->point[3].y + kp->point[4].y) / 2;
    tmp = tmp - kp->point[2].y;

    if (tmp < 0)
        tmp = -tmp;

    percent = (float)((float)tmp / (float)height);
    printf("th3:%.3f\r\n", percent);
    //阈值3
    if (percent < 0.18f) //0.18f
    {
        printf("th3 not pass\r\n");
        ret++;
    }
    sprintf(str, "3:%.4f", percent);
#if DETECT_VERTICAL
    image_rgb565_draw_string(display_image_ver, str, 16, 0, 48,
                             RED, NULL,
                             240, 320);
#else
    image_rgb565_draw_string(display_image, str, 16, 0, 48,
                             RED, NULL,
                             320, 240);
#endif
    ///////////////////////////////////////////////////////////////////////////////
    return (ret > 0) ? 0 : 1;
}
