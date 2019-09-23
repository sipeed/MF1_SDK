#include "demo_esp8285.h"

#include "face_lib.h"
#include "face_cb.h"

#include "net_8285.h"
#include "http_file.h"

#if CONFIG_NET_ENABLE
#if CONFIG_NET_ESP8285
void demo_esp8285(void)
{
    uint64_t tim = 0, last_mqtt_check_tim = 0;

    printf("demo_esp8285\r\n");

    /* init 8285 */
    spi_8266_init_device();

    if (strlen(g_board_cfg.wifi_ssid) > 0 && strlen(g_board_cfg.wifi_passwd) >= 8)
    {
        lcd_display_image_alpha(IMG_CONNING_ADDR, 0);
        g_net_status = spi_8266_connect_ap(g_board_cfg.wifi_ssid, g_board_cfg.wifi_passwd, 2);
        if (g_net_status)
        {
            lcd_display_image_alpha(IMG_CONN_SUCC_ADDR, 0);
#if CONFIG_NET_DEMO_MQTT
            spi_8266_mqtt_init();
#endif /* CONFIG_NET_DEMO_MQTT */
        }
        else
        {
            lcd_display_image_alpha(IMG_CONN_FAILED_ADDR, 0);
            printk("wifi config maybe error,we can not connect to it!\r\n");
        }
    }

    /*
        大致流程:
            开机判断是否有wifi的配置
                有正确的配置则进行联网
                没有正确的配置
                    提醒用户按下按键进行配网
                    然后扫描二维码
                    扫码获取到wifi的配置和进行联网操作
            联网成功就去执行http_get,http_post,mqtt的demo
    */
    while (1)
    {
#if CONFIG_NET_DEMO_HTTP_GET
        uint8_t http_header[1024];
        if (g_net_status)
        {
            dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);

            uint64_t tm = sysctl_get_time_us();

            uint32_t get_recv_len = http_get_file(
                "https://fdvad021asfd8q.oss-cn-hangzhou.aliyuncs.com/logo.jpg", //oss is so slow.
                NULL,
                http_header,
                sizeof(http_header),
                display_image,
                sizeof(display_image));

            uint64_t tt = sysctl_get_time_us() - tm;

            printk("http get use %ld us\r\n", tt);

            float time_s = (float)((float)tt / 1000.0 / 1000.0);
            float file_kb = (float)((float)get_recv_len / 1024.0);

            printf("about %f KB/s\r\n", (float)(file_kb / time_s));

            printk("get_recv_len:%d\r\n", get_recv_len);
            printk("get recv hdr:%s\r\n", http_header);

            if (get_recv_len > 0)
            {
                jpeg_decode_image_t *jpeg = NULL;

                jpeg = pico_jpeg_decode(kpu_image[0], display_image, get_recv_len, 1);

                if (jpeg)
                {
                    printk("img width:%d\theight:%d\r\n", jpeg->width, jpeg->height);
                    if (jpeg->width == 240 && jpeg->height == 240)
                    {
                        convert_jpeg_img_order(jpeg);
                        lcd_draw_picture(0, 0, 240, 240, (uint32_t *)jpeg->img_data);
                    }
                }
#if CONFIG_NET_DEMO_HTTP_POST
                uint8_t *post_send_body = (uint8_t *)malloc(sizeof(uint8_t) * 1024);
                uint8_t *post_send_header = (uint8_t *)malloc(sizeof(uint8_t) * 512);

                uint8_t *boundary = "----WebKitFormBoundaryO2aA3WiAfUqIcD6e"; //header 中要比正文少俩--
                                                                              //结尾又比正文在街位数多俩--
                sprintf(post_send_header, "Sec-Fetch-Mode: cors\r\nUser-Agent: Xel\r\nContent-Type: multipart/form-data; boundary=%s\r\n", boundary);
                sprintf(post_send_body, "--%s\r\nContent-Disposition: form-data; name=\"file\"\r\n\r\nmultipart\r\n--%s\r\nContent-Disposition: form-data; name=\"Filedata\"; filename=\"logo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n", boundary, boundary);

                tm = sysctl_get_time_us();
                uint32_t post_recv_len = http_post_file("http://api.uomg.com/api/image.ali",
                                                        post_send_header,
                                                        post_send_body,
                                                        boundary,
                                                        display_image,
                                                        get_recv_len,
                                                        http_header,
                                                        sizeof(http_header),
                                                        display_image,
                                                        sizeof(display_image));

                tt = sysctl_get_time_us() - tm;

                if (post_recv_len > 0)
                {
                    printk("post_recv_len %d \r\n", post_recv_len);
                    printk("http_header:%s\r\n", http_header);
                    printf("http_body:\r\n%s\r\n", display_image);

                    time_s = (float)((float)tt / 1000.0 / 1000.0);
                    file_kb = (float)((float)get_recv_len / 1024.0);

                    printk("http post use %ld us\r\n", tt);

                    printf("about %f KB/s\r\n", (float)(file_kb / time_s));
                }
#endif /* CONFIG_NET_DEMO_HTTP_POST */
            }

            while (1)
            {
#if CONFIG_NET_DEMO_MQTT
                tim = sysctl_get_time_us();
                /******Process mqtt ********/
                if (g_net_status && !qrcode_get_info_flag)
                {
                    /* mqtt loop */
                    if (PubSubClient_loop() == false)
                    { /* disconnect */
                        mqtt_reconnect();
                    }

                    if (last_mqtt_check_tim < tim)
                    {
                        if (!PubSubClient_connected())
                            mqtt_reconnect();
                        last_mqtt_check_tim += 1000 * 1000; //1000ms
                    }
                }
#endif /* CONFIG_NET_DEMO_MQTT */
            };
        }
#endif /* CONFIG_NET_DEMO_HTTP_GET */

        uint32_t cnt = 0;
        cnt++;
        if (cnt > 0xff)
        {
            cnt = 0;
            printf("should config wifi ssid and passwd \r\n");
        }

        if (!qrcode_get_info_flag)
        {
            memset(display_image, 0, sizeof(display_image));
            image_rgb565_draw_string(display_image, "TOUCH KEY TO CONFIG WiFi...", 16, 40, 0, RED, NULL, 320, 240);
#if (CONFIG_LCD_WIDTH == 240)
            convert_320x240_to_240x240(display_image, 40);
            lcd_draw_picture(0, 0, 240, 240, (uint32_t *)display_image);
#else
            lcd_draw_picture(0, 0, 320, 240, (uint32_t *)display_image);
#endif /* (CONFIG_LCD_WIDTH == 240) */
        }

        /* get key state */
        update_key_state();

        if (g_key_long_press)
        {
            g_key_long_press = 0;
            /* key long press */
            printk("key long press\r\n");
        }

#if CONFIG_KEY_SHORT_QRCODE
        /* key short to scan qrcode */
        if (g_key_press)
        {
            g_key_press = 0;
            qrcode_get_info_flag = 1;
            qrcode_start_time = sysctl_get_time_us();

            set_IR_LED(0);
            open_gc0328_650();
            dvp_set_output_enable(0, 0); //disable to ai
            dvp_set_output_enable(1, 1); //enable to lcd
        }

        if (qrcode_get_info_flag)
        {
            while (!g_dvp_finish_flag)
                ;

            qr_wifi_info_t *wifi_info = qrcode_get_wifi_cfg();
            if (NULL == wifi_info)
            {
                printk("no memory!\r\n");
            }
            else
            {
                switch (wifi_info->ret)
                {
                case QRCODE_RET_CODE_OK:
                {
                    printk("get qrcode\r\n");
                    printk("ssid:%s\tpasswd:%s\r\n", wifi_info->ssid, wifi_info->passwd);

                    if (g_net_status)
                    {
                        printk("already connect net, but want to reconfig, so reboot 8266\r\n");
                        spi_8266_init_device();
                        g_net_status = 0;
                    }

                    //connect to network
                    lcd_display_image_alpha(IMG_CONNING_ADDR, 0);
                    g_net_status = spi_8266_connect_ap(wifi_info->ssid, wifi_info->passwd, 2);
                    if (g_net_status)
                    {
                        lcd_display_image_alpha(IMG_CONN_SUCC_ADDR, 0);
                        spi_8266_mqtt_init();
                        memcpy(g_board_cfg.wifi_ssid, wifi_info->ssid, 32);
                        memcpy(g_board_cfg.wifi_passwd, wifi_info->passwd, 32);
                        if (flash_save_cfg(&g_board_cfg) == 0)
                        {
                            printk("save g_board_cfg failed!\r\n");
                        }
                    }
                    else
                    {
                        lcd_display_image_alpha(IMG_CONN_FAILED_ADDR, 0);
                        printk("wifi config maybe error,we can not connect to it!\r\n");
                    }
                    qrcode_get_info_flag = 0;
                }
                break;
                case QRCODE_RET_CODE_PRASE_ERR:
                {
                    printk("get error qrcode\r\n");
                }
                break;
                case QRCODE_RET_CODE_TIMEOUT:
                {
                    printk("scan qrcode timeout\r\n");
                    /* here display pic */
                    lcd_display_image_alpha(IMG_QR_TIMEOUT_ADDR, 0);
                    qrcode_get_info_flag = 0;
                }
                break;
                case QRCODE_RET_CODE_NO_DATA:
                default:
                    break;
                }
                free(wifi_info);
            }

            /* here display pic */
            lcd_display_image_alpha(IMG_SCAN_QR_ADDR, 160);

            g_dvp_finish_flag = 0;
        }
#endif /* CONFIG_KEY_SHORT_QRCODE */
    }
}
#endif /* CONFIG_NET_ESP8285 */
#endif /* CONFIG_NET_ENABLE */
