#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"

#include "image_op.h"
#include "lcd_dis.h"

#include "lcd.h"

///////////////////////////////////////////////////////////////////////////////
extern void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);

///////////////////////////////////////////////////////////////////////////////
#if CONFIG_LCD_TYPE_SIPEED
static uint8_t l_lcd_image[SIPEED_LCD_W * SIPEED_LCD_H * 2] __attribute__((aligned(64)));                      //显示
static uint8_t l_lcd_banner_image[SIPEED_LCD_BANNER_W * SIPEED_LCD_BANNER_H * 2] __attribute__((aligned(64))); //显示

uint8_t *lcd_banner_image = l_lcd_banner_image;
uint8_t *lcd_image = l_lcd_image;
extern volatile uint8_t dis_flag;
#endif /* CONFIG_LCD_TYPE_SIPEED */

///////////////////////////////////////////////////////////////////////////////
/* protocol record face */
static proto_record_face_cfg_t record_cfg;

static uint8_t proto_record_flag = 0;
static uint64_t proto_record_start_time = 0;

///////////////////////////////////////////////////////////////////////////////
uint8_t delay_flag = 0;

static uint8_t *pDisImage = NULL;
static uint8_t *pCamImage = NULL;
static uint16_t CamImage_W, CamImage_H;
static uint16_t DisLcd_W, DisLcd_H;
static int16_t DisX_Off, DisY_Off;
static int16_t DisImageX_Off, DisImageY_Off;

extern void image_rgb565_draw_line(uint16_t *ptr, uint8_t hor_ver,
                                   uint16_t x, uint16_t y,
                                   uint16_t linew, uint16_t lineh,
                                   uint16_t img_w, uint16_t img_h,
                                   uint16_t color);

//摄像头图像 cam_image
//LCD画布 lcd_image
//先等待cam_image获取完毕
//对于竖屏：进行横竖转换
//对于st7789: 直接作画
//对于sipeed lcd: 先转换像素顺序，再画控件
void face_cb_init(void)
{
    CamImage_W = CONFIG_CAMERA_RESOLUTION_WIDTH;
    CamImage_H = CONFIG_CAMERA_RESOLUTION_HEIGHT;

#if CONFIG_LCD_TYPE_ST7789
    pCamImage = (cam_image);

    DisLcd_W = CONFIG_LCD_WIDTH;
    DisLcd_H = CONFIG_LCD_HEIGHT;

    DisImageX_Off = 0;
    DisImageY_Off = 0;
#else
    //09月25日 星期三
    uint8_t zhCN_date[] = {0x30, 0x39, 0xd4, 0xc2, 0x32, 0x35, 0xc8, 0xd5, 0x20, 0xd0, 0xc7, 0xc6, 0xda, 0xc8, 0xfd, 0x0};
    //深圳XX公司
    uint8_t zhCN_company[] = {0xC9, 0xEE, 0xDb, 0xDA, 0xA2, 0xFA, 0xA2, 0xFA, 0xB9, 0xAB, 0xCB, 0xBE, 0x0};
    //欢迎使用人脸识别
    uint8_t zhCN_welcome[] = {0xBB, 0xB6, 0xD3, 0xAD, 0xCA, 0xB9, 0xD3, 0xC3, 0xC8, 0xCB, 0xC1, 0xB3, 0xCA, 0xB6, 0xB1, 0xF0, 0x0};

    pDisImage = _IOMEM_PADDR(lcd_image);
    pCamImage = _IOMEM_ADDR(cam_image);

    DisLcd_W = SIPEED_LCD_W;
    DisLcd_H = SIPEED_LCD_H;

#if CONFIG_TYPE_800_480_57_INCH
    DisImageX_Off = 0;
    DisImageY_Off = 0;
#if CONFIG_LCD_VERTICAL
    //时间
    image_rgb565_draw_string(lcd_banner_image, "10:08", 48,
                             79, 23, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H);
    //09月25日 星期三
    image_rgb565_draw_zhCN_string(lcd_banner_image, zhCN_date, 16,
                                  79, 77, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H,
                                  lcd_dis_get_zhCN_dat);
    //深圳XX公司
    image_rgb565_draw_zhCN_string(lcd_banner_image, zhCN_company, 24,
                                  67, 106, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H,
                                  lcd_dis_get_zhCN_dat);
    //欢迎使用人脸识别
    image_rgb565_draw_zhCN_string(lcd_banner_image, zhCN_welcome, 16,
                                  279, 105, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H,
                                  lcd_dis_get_zhCN_dat);
    //IP地址
    image_rgb565_draw_string(lcd_banner_image, "192.168.0.169", 16,
                             290, 125, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H);
#else
    //时间
    image_rgb565_draw_string(lcd_banner_image, "10:08", 48,
                             20, 70, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H);
    //09月25日 星期三
    image_rgb565_draw_zhCN_string(lcd_banner_image, zhCN_date, 16,
                                  20, 128, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H,
                                  lcd_dis_get_zhCN_dat);
    //深圳XX公司
    image_rgb565_draw_zhCN_string(lcd_banner_image, zhCN_company, 24,
                                  8, 194, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H,
                                  lcd_dis_get_zhCN_dat);
    //欢迎使用人脸识别
    image_rgb565_draw_zhCN_string(lcd_banner_image, zhCN_welcome, 16,
                                  16, 375, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H,
                                  lcd_dis_get_zhCN_dat);
    //IP地址
    image_rgb565_draw_string(lcd_banner_image, "192.168.0.169", 16,
                             29, 399, WHITE, NULL, SIPEED_LCD_BANNER_W, SIPEED_LCD_BANNER_H);
#endif /* CONFIG_LCD_VERTICAL */
#elif CONFIG_TYPE_480_272_4_3_INCH
    DisImageX_Off = 8;
    DisImageY_Off = 16;
#elif CONFIG_TYPE_1024_600
    DisImageX_Off = 0;
    DisImageY_Off = 0;
#endif /* CONFIG_TYPE_800_480_57_INCH */
#endif /* CONFIG_LCD_TYPE_SIPEED */

    DisX_Off = DisImageX_Off;
#if CONFIG_LCD_VERTICAL
    DisY_Off = DisLcd_W - 16;
#else
    DisY_Off = DisLcd_H - 16;
#endif
    return;
}

#if CONFIG_LCD_TYPE_ST7789
#if (CONFIG_LCD_WIDTH == 240)
void convert_320x240_to_240x240(uint8_t *img_320, uint16_t x_offset)
{
    for (uint8_t i = 0; i < 240; i++)
    {
        memcpy(img_320 + i * 240 * 2, img_320 + i * 320 * 2 + x_offset * 2, 240 * 2);
    }
}
#endif /* (CONFIG_LCD_WIDTH == 240) */
#endif /* CONFIG_LCD_TYPE_ST7789 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void lcd_display_image_alpha(uint32_t pic_addr, uint16_t pic_w, uint16_t pic_h, uint32_t alpha, int16_t oft_x, int16_t oft_y)
{
    lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, pic_addr, alpha, oft_x, oft_y, pic_w, pic_h);

    if (lcd_dis)
    {
        lcd_dis_list_display(pCamImage, CamImage_W, CamImage_H, lcd_dis);
        lcd_dis_list_del_by_id(255);
    }
    else
    {
        image_rgb565_draw_string(pCamImage, "NO MEMRORY...", 16, DisX_Off, DisY_Off, WHITE, NULL, CamImage_W, CamImage_H);
    }

#if CONFIG_LCD_TYPE_SIPEED
    while (dis_flag)
    {
    }; //等待中断刷完屏
    image_rgb565_paste_img(pDisImage, DisLcd_W, DisLcd_H,
                           pCamImage, CamImage_W, CamImage_H,
                           DisImageX_Off, DisImageY_Off);
#else
#if (CONFIG_LCD_WIDTH == 240)
    convert_320x240_to_240x240(pCamImage, oft_x);
#endif

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pCamImage);
#endif /* CONFIG_LCD_TYPE_SIPEED */
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//将摄像头图像 贴/转换 到lcd buf
void lcd_convert_cb(void)
{
#if CONFIG_LCD_TYPE_SIPEED
    convert_rgb565_order((uint16_t *)pCamImage, CamImage_W, CamImage_H);
#endif /* CONFIG_LCD_TYPE_SIPEED */

    if (lcd_dis_list->len)
    {
        list_node_t *node = NULL;
        list_iterator_t *lcd_dis_list_iterator = list_iterator_new(lcd_dis_list, LIST_HEAD);

        if (lcd_dis_list_iterator)
        {
            while ((node = list_iterator_next(lcd_dis_list_iterator)))
            {
                lcd_dis_t *lcd_dis = (lcd_dis_t *)node->val;
                lcd_dis_list_display(pCamImage, CamImage_W, CamImage_H, lcd_dis);
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
#if CONFIG_LCD_TYPE_SIPEED
    while (dis_flag)
    {
    }; //等待中断刷完屏
    image_rgb565_paste_img(pDisImage, DisLcd_W, DisLcd_H,
                           pCamImage, CamImage_W, CamImage_H,
                           DisImageX_Off, DisImageY_Off);
#else
#if (CONFIG_LCD_WIDTH == 240)
    int16_t oft_x = (CamImage_W - PIC_W) / 2 + DisImageX_Off;
    convert_320x240_to_240x240(pCamImage, oft_x);
#endif
    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pCamImage);
#endif /* CONFIG_LCD_TYPE_SIPEED */

    if (delay_flag)
    {
        delay_flag = 0;
        msleep(500);
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
    int16_t oft_x = (CamImage_W - PIC_W) / 2 + DisImageX_Off;
    int16_t oft_y = (CamImage_H - PIC_H) / 2 + DisImageY_Off;
    lcd_display_image_alpha(IMG_FACE_PASS_ADDR, PIC_W, PIC_H, 128, oft_x, oft_y);

    face_lib_draw_flag = 1;
    set_RGB_LED(GLED);
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
        image_rgb565_draw_edge(pCamImage,
                               face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                               BLUE, CamImage_W, CamImage_H);
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
    char str[20];

    uint32_t face_cnt = face->result->face_obj_info.obj_number;

    for (uint32_t i = 0; i < face_cnt; i++)
    {
        face_obj_t *face_info = &(face->result->face_obj_info.obj[i]);
        if (1) //face_info->pass == false)
        {
            image_rgb565_draw_edge((pCamImage),
                                   face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                                   RED, CamImage_W, CamImage_H);

            uint16_t bg = 0;
            int16_t oft_x = (CamImage_W - PIC_W) / 2 + DisImageX_Off;

            sprintf(str, "%.3f", face_info->score);
            image_rgb565_draw_string(pCamImage, str, 16,
                                     oft_x, DisY_Off,
                                     RED, &bg,
                                     CamImage_W, CamImage_H);
        }

        sprintf(str, "%.4f", face_info->prob);
        printk("face prob:%s\r\n", str);

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
                printk("record face\r\n");

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
                printk("#######save face\r\n");
                if (flash_save_face_info(&face_fea, record_cfg.uid, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
                {
                    printk("Feature Full\n");
                    record_cfg.send_ret(1, "flash is full", NULL);
                    break;
                }

                record_cfg.send_ret(0, "record face success", record_cfg.uid);

                proto_record_flag = 0;

                int16_t oft_x = (CamImage_W - PIC_W) / 2 + DisImageX_Off;
                int16_t oft_y = (CamImage_H - PIC_H) / 2 + DisImageY_Off;
                lcd_display_image_alpha(IMG_RECORD_FACE_ADDR, PIC_W, PIC_H, 128, oft_x, oft_y);
                face_lib_draw_flag = 1;

                set_RGB_LED(GLED);
                msleep(500); //this delay can modify
                set_RGB_LED(0);
            }
        }
        else
        {
            /* 按键录入的功能可以删除 */
            if (g_key_press)
            {
                /* only one person then record face ???? */
                if (face_info->feature)
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

                    printk("#######save face\r\n");
                    if (flash_save_face_info(&face_fea, NULL, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
                    {
                        printk("Feature Full\n");
                        break;
                    }

                    int16_t oft_x = (CamImage_W - PIC_W) / 2 + DisImageX_Off;
                    int16_t oft_y = (CamImage_H - PIC_H) / 2 + DisImageY_Off;
                    lcd_display_image_alpha(IMG_RECORD_FACE_ADDR, PIC_W, PIC_H, 128, oft_x, oft_y);
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
_exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if CONFI_SINGLE_CAMERA
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
            image_rgb565_draw_edge(pCamImage,
                                   face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                                   YELLOW, CamImage_W, CamImage_H);
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
                image_rgb565_draw_edge(pCamImage,
                                       face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                                       GREEN, CamImage_W, CamImage_H);
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

    if (ir_check)
    {
        face_cnt = face->result->face_compare_info.result_number;

        for (uint32_t i = 0; i < face_cnt; i++)
        {
            face_info = (face_obj_t *)(face->result->face_compare_info.obj[i]);
            image_rgb565_draw_edge(pCamImage,
                                   face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                                   GREEN, CamImage_W, CamImage_H);
            face_pass_callback(face_info, face_cnt, i, &tim);
        }
    }
    else
    {
        face_cnt = face->result->face_obj_info.obj_number;

        //FIXME:这里需要输出信息？？
        for (uint32_t i = 0; i < face_cnt; i++)
        {
            if (g_board_cfg.brd_soft_cfg.cfg.out_fea == 2)
            {
                face_info = (face_obj_t *)&(face->result->face_obj_info.obj[i]);
                face_pass_callback(face_info, face_cnt, i, &tim);
                image_rgb565_draw_edge(pCamImage,
                                       face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                                       WHITE, CamImage_W, CamImage_H);
            }
            else
            {
                printk("unknow\r\n");
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

    printk("th1:%d\r\n", tmp);

    //阈值1
    if (tmp > 8)
    {
        printk("th2 not pass\r\n");
        ret++;
    }

    //     sprintf(str, "1:%d", tmp);
    // #if DETECT_VERTICAL
    //     image_rgb565_draw_string(pDisImage, str, 16, 0, 16,
    //                              RED, NULL,
    //                              240, 320);
    // #else
    //     image_rgb565_draw_string(pDisImage, str, 16, 0, 16,
    //                              RED, NULL,
    //                              320, 240);
    // #endif

    ///////////////////////////////////////////////////////////////////////////////
    tmp = (kp->point[0].y + kp->point[1].y) / 2;
    tmp = kp->point[2].y - tmp;

    if (tmp < 0)
        tmp = -tmp;

    percent = (float)((float)tmp / (float)height);
    printk("th2:%.3f\r\n", percent);

    //阈值2
    if (percent < 0.14f) //0.14f
    {
        printk("th2 not pass\r\n");
        ret++;
    }

    //     sprintf(str, "2:%.4f", percent);
    // #if DETECT_VERTICAL
    //     image_rgb565_draw_string(lcd_image, str, 16, 0, 32,
    //                              RED, NULL,
    //                              240, 320);
    // #else
    //     image_rgb565_draw_string(pCamImage, str, 16, 0, 32,
    //                              RED, NULL,
    //                              320, 240);
    // #endif

    ///////////////////////////////////////////////////////////////////////////////
    tmp = (kp->point[3].y + kp->point[4].y) / 2;
    tmp = tmp - kp->point[2].y;

    if (tmp < 0)
        tmp = -tmp;

    percent = (float)((float)tmp / (float)height);
    printk("th3:%.3f\r\n", percent);
    //阈值3
    if (percent < 0.18f) //0.18f
    {
        printk("th3 not pass\r\n");
        ret++;
    }
    //     sprintf(str, "3:%.4f", percent);
    // #if DETECT_VERTICAL
    //     image_rgb565_draw_string(pDisImage, str, 16, 0, 48,
    //                              RED, NULL,
    //                              240, 320);
    // #else
    //     image_rgb565_draw_string(pCamImage, str, 16, 0, 48,
    //                              RED, NULL,
    //                              320, 240);
    // #endif
    ///////////////////////////////////////////////////////////////////////////////
    return (ret > 0) ? 0 : 1;
}
