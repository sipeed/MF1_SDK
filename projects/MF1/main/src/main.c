#include "face_lib.h"

#include "global_build_info_version.h"

#include "face_cb.h"
#include "camera.h"

#include "lcd.h"
#include "lcd_dis.h"

#if CONFIG_ENABLE_OUTPUT_JPEG
#include "cQueue.h"
#include "core1.h"
#endif /* CONFIG_ENABLE_OUTPUT_JPEG */

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint64_t last_open_relay_time_in_s = 0;
static volatile uint8_t relay_open_flag = 0;
static uint64_t last_pass_time = 0;

static void uart_send(char *buf, size_t len);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
face_recognition_cfg_t face_recognition_cfg = {
    .check_ir_face = 1,
    .auto_out_fea = 0,
    .detect_threshold = 0.0,
    .compare_threshold = 0.0,

#if CONFIG_ENABLE_FLASH_LED
    .use_flash_led = 1,
#else
    .use_flash_led = 0,
#endif

    .no_face_ctrl_lcd = 0,
};

face_lib_callback_t face_recognition_cb = (face_lib_callback_t){
    .proto_send = uart_send,
    .proto_record_face = protocol_record_face,

    .detected_face_cb = detected_face_cb,
    .fake_face_cb = fake_face_cb,
    .pass_face_cb = pass_face_cb,

    .lcd_refresh_cb = lcd_refresh_cb,
    .lcd_convert_cb = lcd_convert_cb,
    .lcd_close_bl_cb = lcd_close_bl_cb,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void uart_send(char *buf, size_t len)
{
    uart_send_data(PROTOCOL_UART_NUM, buf, len);
}

static void open_relay(void)
{
    uint64_t tim = sysctl_get_time_us();

    gpiohs_set_pin(CONFIG_RELAY_LOWX_GPIOHS_NUM, 1);
    gpiohs_set_pin(CONFIG_RELAY_HIGH_GPIOHS_NUM, 0);

    last_open_relay_time_in_s = tim / 1000 / 1000;
    relay_open_flag = 1;
}

static void close_relay(void)
{
    gpiohs_set_pin(CONFIG_RELAY_LOWX_GPIOHS_NUM, 0);
    gpiohs_set_pin(CONFIG_RELAY_HIGH_GPIOHS_NUM, 1);
}

/********************* Add your callback code here *********************/
void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time)
{
    face_info_t info;
    uint64_t tim = 0;

    tim = sysctl_get_time_us();

    if (g_board_cfg.brd_soft_cfg.cfg.out_interval_ms != 0)
    {
        if (((tim - last_pass_time) / 1000) < g_board_cfg.brd_soft_cfg.cfg.out_interval_ms)
        {
            printk("last face pass time too short\r\n");
            return;
        }
    }

    last_pass_time = tim;

    /* output feature */
    if (g_board_cfg.brd_soft_cfg.cfg.auto_out_fea)
    {
        protocol_send_face_info(obj,
                                0, NULL, obj->feature,
                                total, current, time);
        open_relay(); //open when have face
    }
    else
    {
        if (obj->score > g_board_cfg.brd_soft_cfg.out_threshold)
        {
            open_relay(); //open when score > gate
            if (flash_get_saved_faceinfo(&info, obj->index) == 0)
            {
                if (g_board_cfg.brd_soft_cfg.cfg.out_fea == 2)
                {
                    //output real time feature
                    protocol_send_face_info(obj,
                                            obj->score, info.uid, g_board_cfg.brd_soft_cfg.cfg.out_fea ? obj->feature : NULL,
                                            total, current, time);
                }
                else
                {
                    //output stored in flash face feature
                    face_fea_t *face_fea = (face_fea_t *)&(info.info);
                    protocol_send_face_info(obj,
                                            obj->score, info.uid,
                                            g_board_cfg.brd_soft_cfg.cfg.out_fea ? (face_fea->stat == 1) ? face_fea->fea_ir : face_fea->fea_rgb : NULL,
                                            total, current, time);
                }
            }
            else
            {
                printk("index error!\r\n");
            }
        }
        else
        {
            printk("face score not pass\r\n");
        }
    }

    if (g_board_cfg.brd_soft_cfg.cfg.auto_out_fea == 0)
    {
        lcd_draw_pass();
    }
    return;
}

//recv: {"version":1,"type":"test"}
//send: {"version":1,"type":"test","code":0,"msg":"test"}
void test_cmd(cJSON *root)
{
    cJSON *ret = protocol_gen_header("test", 0, "test");
    if (ret)
    {
        protocol_send(ret);
    }
    cJSON_Delete(ret);
    return;
}

//recv: {"version":1,"type":"test2","log_tx":10}
//send: {"1":1,"type":"test2","code":0,"msg":"test2"}
void test2_cmd(cJSON *root)
{
    cJSON *ret = NULL;
    cJSON *tmp = NULL;

    tmp = cJSON_GetObjectItem(root, "log_tx");
    if (tmp == NULL)
    {
        printk("no log_tx recv\r\n");
    }
    else
    {
        printk("log_tx:%d\r\n", tmp->valueint);
    }

    ret = protocol_gen_header("test2", 0, "test2");
    if (ret)
    {
        protocol_send(ret);
    }
    cJSON_Delete(ret);
    return;
}

protocol_custom_cb_t user_custom_cmd[] = {
    {.cmd = "test", .cmd_cb = test_cmd},
    {.cmd = "test2", .cmd_cb = test2_cmd},
};

int main(void)
{
    uint64_t tim = 0;

    board_init();
    face_lib_init_module();

    face_cb_init();
    lcd_dis_list_init();

    printk("firmware version:\r\n%d.%d.%d\r\n", BUILD_VERSION_MAJOR, BUILD_VERSION_MINOR, BUILD_VERSION_MICRO);
    printk("face_lib_version:\r\n%s\r\n", face_lib_version());

    /*load cfg from flash*/
    if (flash_load_cfg(&g_board_cfg) == 0)
    {
        printk("load cfg failed,save default config\r\n");

        flash_cfg_set_default(&g_board_cfg);

        if (flash_save_cfg(&g_board_cfg) == 0)
        {
            printk("save g_board_cfg failed!\r\n");
        }
    }

    flash_cfg_print(&g_board_cfg);

    face_lib_regisiter_callback(&face_recognition_cb);

#if CONFIG_NET_ENABLE
    protocol_init_device(&g_board_cfg, 1);
#if CONFIG_NET_ESP8285
    extern void demo_esp8285(void);
    demo_esp8285();
#elif CONFIG_NET_W5500
    extern void demo_w5500(void);
    demo_w5500();
#endif
#endif

#if CONFIG_ENABLE_OUTPUT_JPEG
    /* 这个需要可配置 */
    fpioa_set_function(CONFIG_JPEG_OUTPUT_PORT_TX, FUNC_UART1_TX + UART_DEV2 * 2);
    fpioa_set_function(CONFIG_JPEG_OUTPUT_PORT_RX, FUNC_UART1_RX + UART_DEV2 * 2);
    uart_config(UART_DEV2, CONFIG_OUTPUT_JPEG_UART_BAUD, 8, UART_STOP_1, UART_PARITY_NONE);

    printk("jpeg output uart cfg:\r\n");
    printk("                     tx:%d rx:%d, baud:%d\r\n",
           CONFIG_JPEG_OUTPUT_PORT_TX, CONFIG_JPEG_OUTPUT_PORT_RX, CONFIG_OUTPUT_JPEG_UART_BAUD);
    q_init(&q_core1, sizeof(void *), 10, FIFO, false); //core1 接受core0 上传图片到服务器的队列
    register_core1(core1_function, NULL);
#endif /* CONFIG_ENABLE_OUTPUT_JPEG */

    /* init device */
    protocol_regesiter_user_cb(&user_custom_cmd[0], sizeof(user_custom_cmd) / sizeof(user_custom_cmd[0]));
    protocol_init_device(&g_board_cfg, 0);
    protocol_send_init_done();

    while (1)
    {
        if (!jpeg_recv_start_flag)
        {
            face_recognition_cfg.auto_out_fea = (uint8_t)g_board_cfg.brd_soft_cfg.cfg.auto_out_fea;
            face_recognition_cfg.compare_threshold = (float)g_board_cfg.brd_soft_cfg.out_threshold;
            face_lib_run(&face_recognition_cfg);
        }

        /* get key state */
        update_key_state();

        if (g_key_long_press)
        {
            g_key_long_press = 0;
            /* key long press */
            printk("key long press\r\n");

#if CONFIG_LONG_PRESS_FUNCTION_KEY_RESTORE
            /* set cfg to default */
            printk("reset board config\r\n");
            board_cfg_t board_cfg;

            memset(&board_cfg, 0, sizeof(board_cfg_t));

            flash_cfg_set_default(&board_cfg);

            if (flash_save_cfg(&board_cfg) == 0)
            {
                printk("save board_cfg failed!\r\n");
            }
            /* set cfg to default end */
#if CONFIG_LONG_PRESS_FUNCTION_KEY_CLEAR_FEATURE
            printk("Del All feature!\n");
            flash_delete_face_all();
#endif
            char *str_del = (char *)malloc(sizeof(char) * 32);
            sprintf(str_del, "Factory Reset...");
            if (lcd_dis_list_add_str(1, 1, 16, 0, str_del, 0, 0, RED, 1) == NULL)
            {
                printk("add dis str failed\r\n");
            }
            delay_flag = 1;
#endif
        }
        /******Process relay open********/
        tim = sysctl_get_time_us();
        tim = tim / 1000 / 1000;

        if (relay_open_flag && ((tim - last_open_relay_time_in_s) >= g_board_cfg.brd_soft_cfg.cfg.relay_open_s))
        {
            close_relay();
            relay_open_flag = 0;
        }

        /******Process uart protocol********/
        if (recv_over_flag)
        {
            protocol_prase(cJSON_prase_buf);
            recv_over_flag = 0;
        }

        if (lcd_bl_stat == 0)
        {
            static uint8_t led_cnt = 0, led_stat = 0;
            led_cnt++;
            if (led_cnt >= 10)
            {
                led_cnt = 0;
                led_stat ^= 0x1;
                set_RGB_LED(led_stat ? GLED : 0);
            }
        }

        if (jpeg_recv_start_flag)
        {
            tim = sysctl_get_time_us();
            if (tim - jpeg_recv_start_time >= 10 * 1000 * 1000) //FIXME: 10s or 5s timeout
            {
                printk("abort to recv jpeg file\r\n");
                jpeg_recv_start_flag = 0;
                protocol_stop_recv_jpeg();
                protocol_send_cal_pic_result(7, "timeout to recv jpeg file", NULL, NULL, 0); //7  jpeg verify error
            }

            /* recv over */
            if (jpeg_recv_len != 0)
            {
                protocol_cal_pic_fea(&cal_pic_cfg, protocol_send_cal_pic_result);
                jpeg_recv_len = 0;
                jpeg_recv_start_flag = 0;
            }
        }
    }
}
