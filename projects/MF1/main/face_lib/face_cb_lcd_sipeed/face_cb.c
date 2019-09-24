#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"

#include "image_op.h"
#include "lcd_dis.h"

#include "lcd.h"
///////////////////////////////////////////////////////////////////////////////
//旋转或者显示宽度大于摄像头宽度时需要额外lcd显存
//统一使用lcd_image来指示lcd缓存
static uint8_t l_lcd_image[SIPEED_LCD_W * SIPEED_LCD_H * 2] __attribute__((aligned(64)));                      //显示
static uint8_t l_lcd_banner_image[SIPEED_LCD_BANNER_W * SIPEED_LCD_BANNER_H * 2] __attribute__((aligned(64))); //显示

uint8_t *lcd_image = l_lcd_image;
uint8_t *lcd_banner_image = l_lcd_banner_image;

///////////////////////////////////////////////////////////////////////////////
extern volatile uint8_t dis_flag;
extern void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);

///////////////////////////////////////////////////////////////////////////////
uint8_t delay_flag = 0;

static uint8_t *pDisImage = NULL;
static uint8_t *pCamImage = NULL;
static uint16_t CamImage_W, CamImage_H;
static uint16_t DisLcd_W, DisLcd_H;
static int16_t DisX_Off, DisY_Off;
static int16_t DisImageX_Off, DisImageY_Off;

//摄像头图像 cam_image
//LCD画布 lcd_image
//先等待cam_image获取完毕
//对于竖屏：进行横竖转换
//对于st7789: 直接作画
//对于sipeed lcd: 先转换像素顺序，再画控件
void face_cb_init(void)
{
    pCamImage = _IOMEM_ADDR(cam_image);
    pDisImage = _IOMEM_PADDR(lcd_image);

    CamImage_W = CONFIG_CAMERA_RESOLUTION_WIDTH;
    CamImage_H = CONFIG_CAMERA_RESOLUTION_HEIGHT;

    DisLcd_W = SIPEED_LCD_W;
    DisLcd_H = SIPEED_LCD_H;

#if CONFIG_TYPE_800_480_57_INCH
    DisImageX_Off = 0;
    DisImageY_Off = 0;
#elif CONFIG_TYPE_480_272_4_3_INCH
    DisImageX_Off = 8;
    DisImageY_Off = 16;
#elif CONFIG_TYPE_1024_600
    DisImageX_Off = 0;
    DisImageY_Off = 0;
#endif

    DisX_Off = DisImageX_Off;
    DisY_Off = DisLcd_H - 16;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//将摄像头图像 贴/转换 到lcd buf
void convert_cam2lcd(uint8_t *lcd_img, uint8_t *cam_img)
{
//竖屏
#if CONFIG_DETECT_VERTICAL
#if CONFIG_LCD_TYPE_ST7789
    //5300us @ 400M
    convert_rgb565_order((uint16_t *)cam_img, CamImage_W, CamImage_H);                            //转换像素顺序
    image_rgb565_roate_right90((uint16_t *)lcd_img, (uint16_t *)cam_img, CamImage_W, CamImage_H); //旋转90度
    convert_rgb565_order((uint16_t *)lcd_img, CamImage_H, CamImage_W);                            //转换回去
    return;
#else
    configASSERT(0); //TODO: 大屏的竖屏
#endif /*CONFIG_LCD_TYPE_ST7789*/
#else
    //横屏
    convert_rgb565_order(cam_img, CamImage_W, CamImage_H); //调整像素顺序
    while (dis_flag)
    {
    }; //等待中断刷完屏
#endif /*CONFIG_DETECT_VERTICAL*/

    image_rgb565_paste_img(lcd_img, DisLcd_W, DisLcd_H,
                           cam_img, CamImage_W, CamImage_H,
                           DisImageX_Off, DisImageY_Off);

    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//摄像头图像转换到lcd图像，并贴图
void prepare_lcd_image(void)
{
    //1. 摄像头缓存转换为lcd缓存
    convert_cam2lcd(pDisImage, pCamImage);

    //2. 遍历lcd显示链表, 贴图
    if (lcd_dis_list->len)
    {
        list_node_t *node = NULL;
        list_iterator_t *lcd_dis_list_iterator = list_iterator_new(lcd_dis_list, LIST_HEAD);

        if (lcd_dis_list_iterator)
        {
            while ((node = list_iterator_next(lcd_dis_list_iterator)))
            {
                lcd_dis_t *lcd_dis = (lcd_dis_t *)node->val;
                lcd_dis_list_display(pDisImage, DisLcd_W, DisLcd_H, lcd_dis);
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
    prepare_lcd_image();
    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);
    //printk("end lcd@%ld\r\n", sysctl_get_time_us()/1000);	//正常刷屏使用约10ms
    if (delay_flag)
    {
        delay_flag = 0;
        msleep(500);
    }
    return;
}

void lcd_display_image_alpha(uint32_t pic_addr, uint16_t pic_w, uint16_t pic_h, uint32_t alpha, int16_t oft_x, int16_t oft_y)
{
    prepare_lcd_image();
    lcd_dis_t *lcd_dis = lcd_dis_list_add_pic(255, 1, pic_addr, alpha, oft_x, oft_y, pic_w, pic_h);

    if (lcd_dis)
    {
        lcd_dis_list_display(pDisImage, DisLcd_W, DisLcd_H, lcd_dis);
        // printk("L%d@%ld\r\n", __LINE__, sysctl_get_time_us() / 1000);
        lcd_dis_list_del_by_id(255);
    }
    else
    {
        image_rgb565_draw_string(pDisImage, "NO MEMRORY...", 16, DisX_Off, DisY_Off, WHITE, NULL, CamImage_W, CamImage_H);
    }

    lcd_draw_picture(0, 0, DisLcd_W, DisLcd_H, (uint32_t *)pDisImage);
    return;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void lcd_draw_pass(void)
{
    int16_t oft_x = (CONFIG_CAMERA_RESOLUTION_HEIGHT - PIC_W) / 2 + DisImageX_Off;
    int16_t oft_y = (CONFIG_CAMERA_RESOLUTION_WIDTH - PIC_H) / 2 + DisImageY_Off;
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
static proto_record_face_cfg_t record_cfg;

static uint8_t proto_record_flag = 0;
static uint64_t proto_record_start_time = 0;

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
        if (1) //face_info->pass == false)
        {
            //printk("false face\r\n");
            image_rgb565_draw_edge((pCamImage),
                                   face_info->x1, face_info->y1, face_info->x2, face_info->y2,
                                   RED, CamImage_W, CamImage_H);

            uint16_t bg = 0x0;
            char str[20];
            sprintf(str, "%.3f", face_info->score);
            image_rgb565_draw_string((pCamImage), str, 16,
                                     0, DisY_Off,
                                     WHITE, &bg,
                                     CamImage_W, CamImage_H);
        }

        //printk("face prob:%.4f\r\n", face_info->prob);

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

                int16_t oft_x = (CONFIG_CAMERA_RESOLUTION_HEIGHT - PIC_W) / 2 + DisImageX_Off;
                int16_t oft_y = (CONFIG_CAMERA_RESOLUTION_WIDTH - PIC_H) / 2 + DisImageY_Off;
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

                    int16_t oft_x = (CONFIG_CAMERA_RESOLUTION_HEIGHT - PIC_W) / 2 + DisImageX_Off;
                    int16_t oft_y = (CONFIG_CAMERA_RESOLUTION_WIDTH - PIC_H) / 2 + DisImageY_Off;
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

    sprintf(str, "1:%d", tmp);
#if DETECT_VERTICAL
    image_rgb565_draw_string(pDisImage, str, 16, 0, 16,
                             RED, NULL,
                             240, 320);
#else
    image_rgb565_draw_string(pDisImage, str, 16, 0, 16,
                             RED, NULL,
                             320, 240);
#endif

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

    sprintf(str, "2:%.4f", percent);
#if DETECT_VERTICAL
    image_rgb565_draw_string(lcd_image, str, 16, 0, 32,
                             RED, NULL,
                             240, 320);
#else
    image_rgb565_draw_string(pCamImage, str, 16, 0, 32,
                             RED, NULL,
                             320, 240);
#endif

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
    sprintf(str, "3:%.4f", percent);
#if DETECT_VERTICAL
    image_rgb565_draw_string(pDisImage, str, 16, 0, 48,
                             RED, NULL,
                             240, 320);
#else
    image_rgb565_draw_string(pCamImage, str, 16, 0, 48,
                             RED, NULL,
                             320, 240);
#endif
    ///////////////////////////////////////////////////////////////////////////////
    return (ret > 0) ? 0 : 1;
}
