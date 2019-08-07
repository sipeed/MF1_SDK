#include "face_lib.h"

#include "draw.h"

#include "ff.h"

#include "face_cb.h"
#include "img_op.h"
#include "sd_op.h"

#if CONFIG_LCD_TYPE_ST7789
#include "lcd_st7789.h"
#elif CONFIG_LCD_TYPE_SSD1963
#include "lcd_ssd1963.h"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DBG_650_IMG 0
#define DBG_850_IMG 0

extern volatile uint8_t g_key_long_press;

void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);

static uint64_t last_open_relay_time_in_s = 0;
static volatile uint8_t relay_open_flag = 0;

face_recognition_cfg_t face_recognition_cfg = {
    .check_ir_face = 1,
    .lcd_type = 1,
    .auto_out_fea = 0,
    .detect_threshold = 0.0,
    .compare_threshold = 0.0,
};

face_lib_callback_t face_recognition_cb = {
    .lcd_draw_picture = recognition_draw_callback,
    .lcd_draw_edge = draw_edge,
    .lcd_box_false_face = box_false_face,
    .record_face = record_face,
    .face_pass_cb = face_pass_callback,
};

static void open_relay(void)
{
    uint64_t tim = sysctl_get_time_us();

    gpiohs_set_pin(RELAY_LOWX_HS_NUM, RELAY_LOWX_OPEN);
    gpiohs_set_pin(RELAY_HIGH_HS_NUM, RELAY_HIGH_OPEN);

    last_open_relay_time_in_s = tim / 1000 / 1000;
    relay_open_flag = 1;
}

static void close_relay(void)
{
    gpiohs_set_pin(RELAY_LOWX_HS_NUM, !RELAY_LOWX_OPEN);
    gpiohs_set_pin(RELAY_HIGH_HS_NUM, !RELAY_HIGH_OPEN);
}

static uint64_t last_pass_time = 0;

/********************* Add your callback code here *********************/
void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time)
{
    face_info_t info;
    uint64_t tim = 0;

    tim = sysctl_get_time_us();

    if(g_board_cfg.out_interval_in_ms != 0)
    {
        if(((tim - last_pass_time) / 1000) < g_board_cfg.out_interval_in_ms)
        {
            printf("last face pass time too short\r\n");
            return;
        }
    }

    last_pass_time = tim;

#if CONFIG_LCD_TYPE_ST7789
    utils_display_pass();
#endif

    open_relay();

    /* output feature */
    if(g_board_cfg.auto_out_feature == 1)
    {
        protocol_send_face_info(obj,
                                0, NULL, obj->feature,
                                total, current, time);
        // open_relay(); //open when have face
    } else
    {
        if(flash_get_saved_faceinfo(&info, obj->index) == 0)
        {
            if((g_board_cfg.auto_out_feature >> 1) & 1)
            {
                //output real time feature
                protocol_send_face_info(obj,
                                        obj->score, info.uid, obj->feature,
                                        total, current, time);
            } else
            {
                //output feature in flash
                protocol_send_face_info(obj,
                                        obj->score, info.uid, info.info,
                                        total, current, time);
            }

            // if(obj->score >= g_board_cfg.face_gate)
            // {
            //     open_relay(); //open when score > gate
            // }
        } else
        {
            printf("index error!\r\n");
        }
    }
    return;
}

int main(void)
{
    uint64_t tim = 0;

    board_init();
    sd_init_fatfs();
    face_lib_init_module();

    /*load cfg from flash*/
    if(flash_load_cfg(&g_board_cfg))
    {
        printf("load cfg %s\r\n", g_board_cfg.cfg_right_flag ? "success" : "error");
        flash_cfg_print(&g_board_cfg);
    } else
    {
        printf("load cfg failed,save default config\r\n");

        flash_cfg_set_default(&g_board_cfg);

        if(flash_save_cfg(&g_board_cfg) == 0)
        {
            printf("save g_board_cfg failed!\r\n");
        }
    }

    /* recognition threshold */
    face_recognition_cfg.compare_threshold = (float)g_board_cfg.face_gate;
    printf("set compare_threshold: %d \r\n", g_board_cfg.face_gate);

    /* init device */
    protocol_init_device(&g_board_cfg);
    init_relay_key_pin(g_board_cfg.key_relay_pin_cfg);
    protocol_send_init_done();

    face_lib_regisiter_callback(&face_recognition_cb);

    while(1)
    {
        /* if rcv jpg, will stuck a period */
        if(!jpeg_recv_start_flag)
        {
            face_lib_run(&face_recognition_cfg);
            update_key_state();
        }

        if(g_key_long_press)
        {
            g_key_long_press = 0;
            /* key long press */
            printf("key long press\r\n");
#if CONFIG_KEY_LONG_CLEAR_FEA
            printf("Del All feature!\n");
            flash_delete_face_all();
#if CONFIG_LCD_TYPE_ST7789

#elif CONFIG_LCD_TYPE_SSD1963
            lcd_draw_string(LCD_OFT, IMG_H - 16, "Del All feature!", lcd_color(0xff, 0, 0));
#endif
            set_RGB_LED(RLED);
            msleep(500);
            msleep(500);
            set_RGB_LED(0);
#endif

#if CONFIG_KEY_LONG_RESTORE
            /* set cfg to default */
            printf("reset board config\r\n");
            board_cfg_t board_cfg;

            memset(&board_cfg, 0, sizeof(board_cfg_t));

            flash_cfg_set_default(&board_cfg);

            if(flash_save_cfg(&board_cfg) == 0)
            {
                printf("save board_cfg failed!\r\n");
            }
            /* set cfg to default end */
#endif
        }

        /******Process relay open********/
        tim = sysctl_get_time_us();
        tim = tim / 1000 / 1000;

        if(relay_open_flag && ((tim - last_open_relay_time_in_s) >= g_board_cfg.relay_open_in_s))
        {
            close_relay();
            relay_open_flag = 0;
        }

        /******Process uart protocol********/
        if(recv_over_flag)
        {
            protocol_prase(cJSON_prase_buf);
            recv_over_flag = 0;
        }

        if(jpeg_recv_start_flag)
        {
            tim = sysctl_get_time_us();
            if(tim - jpeg_recv_start_time >= 10 * 1000 * 1000) //FIXME: 10s or 5s timeout
            {
                printf("abort to recv jpeg file\r\n");
                jpeg_recv_start_flag = 0;
                protocol_stop_recv_jpeg();
                protocol_send_cal_pic_result(7, "timeout to recv jpeg file", NULL, NULL, 0); //7  jpeg verify error
            }

            /* recv over */
            if(jpeg_recv_len != 0)
            {
                protocol_cal_pic_fea(jpeg_recv_buf, jpeg_recv_len);
                jpeg_recv_len = 0;
            }
        }
    }
}
