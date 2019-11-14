#include "uart_recv.h"
#include "board.h"
#include "face_lib.h"
#include "flash.h"
#include "lcd.h"
#include "camera.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
volatile uint8_t recv_over_flag = 0;
volatile uint8_t jpeg_recv_start_flag = 0;

pkt_head_t g_pkt_head;

uint32_t jpeg_recv_len = 0;
uint64_t jpeg_recv_start_time = 0;

uint8_t cJSON_prase_buf[PROTOCOL_BUF_LEN];
uint8_t jpeg_recv_buf[JPEG_BUF_LEN];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static unsigned char cJSON_recv_buf[PROTOCOL_BUF_LEN];

static volatile uint8_t start_recv_flag = 0;
static volatile uint32_t recv_cur_pos = 0;

static volatile uint8_t recv_jpeg_flag = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t uart_channel_getchar(uart_device_number_t channel, uint8_t *data);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int protocol_port_recv_cb(void *ctx)
{
    static uint8_t last_data = 0;

    uint8_t tmp;
    do
    {
        if (uart_channel_getchar(PROTOCOL_UART_NUM, &tmp) == 0)
            return 0;

        /* recv jpeg */
        if (recv_jpeg_flag)
        {
            //接受jpeg图片
            if (start_recv_flag)
            {
                jpeg_recv_buf[recv_cur_pos++] = tmp;

                if ((tmp == g_pkt_head.pkt_jpeg_end_2) && ((jpeg_recv_buf[recv_cur_pos - 2] == g_pkt_head.pkt_jpeg_end_1) || (recv_cur_pos > JPEG_BUF_LEN - 1)))
                {
                    start_recv_flag = 0;
                    recv_jpeg_flag = 0;
                    if (g_board_cfg.brd_soft_cfg.cfg.pkt_fix)
                    {
                        jpeg_recv_len = recv_cur_pos - 2;
                    }
                    else
                    {
                        jpeg_recv_len = recv_cur_pos;
                    }
                    return 0;
                }
            }
            else
            {
                if ((tmp == g_pkt_head.pkt_jpeg_start_2) && (last_data == g_pkt_head.pkt_jpeg_start_1))
                {
                    recv_cur_pos = 0;
                    if (g_board_cfg.brd_soft_cfg.cfg.pkt_fix == 0)
                    {
                        jpeg_recv_buf[recv_cur_pos++] = 0xff;
                        jpeg_recv_buf[recv_cur_pos++] = 0xd8;
                    }
                    start_recv_flag = 1;
                    last_data = 0;
                }
                else
                {
                    last_data = tmp;
                }
            }
        }
        /* recv json */
        else
        {
            if (start_recv_flag)
            {
                cJSON_recv_buf[recv_cur_pos++] = tmp;

                if ((tmp == g_pkt_head.pkt_json_end_2) && ((cJSON_recv_buf[recv_cur_pos - 2] == g_pkt_head.pkt_json_end_1) || (recv_cur_pos > PROTOCOL_BUF_LEN - 1)))
                {
                    memcpy(cJSON_prase_buf, cJSON_recv_buf, recv_cur_pos - 2);
                    memset(cJSON_recv_buf, 0, sizeof(cJSON_recv_buf));
                    cJSON_prase_buf[recv_cur_pos - 2] = 0;
                    recv_over_flag = 1;
                    start_recv_flag = 0;
                    return 0;
                }
            }
            else
            {
                if (g_board_cfg.brd_soft_cfg.cfg.pkt_fix)
                {
                    if ((tmp == g_pkt_head.pkt_json_start_2) && (last_data == g_pkt_head.pkt_json_start_1))
                    {
                        recv_cur_pos = 0;
                        cJSON_recv_buf[recv_cur_pos++] = g_pkt_head.pkt_json_start_1;
                        cJSON_recv_buf[recv_cur_pos++] = g_pkt_head.pkt_json_start_2;
                        start_recv_flag = 1;
                        last_data = 0;
                    }
                    else
                    {
                        last_data = tmp;
                    }
                }
                else
                {
                    if (tmp == '{')
                    {
                        recv_cur_pos = 0;
                        cJSON_recv_buf[recv_cur_pos++] = tmp;
                        start_recv_flag = 1;
                    }
                }
            }
        }
        /* recv end */
    } while (1);

    return 0;
}

static void init_board_uart_port(board_cfg_t *brd_cfg)
{
    uint8_t port_tx = 0, port_rx = 0;
    uint8_t log_tx = 0, log_rx = 0;

    port_tx = brd_cfg->brd_hard_cfg.uart_relay_key.port_tx;
    port_rx = brd_cfg->brd_hard_cfg.uart_relay_key.port_rx;
    log_tx = brd_cfg->brd_hard_cfg.uart_relay_key.log_tx;
    log_rx = brd_cfg->brd_hard_cfg.uart_relay_key.log_rx;

    printk("board uart pin cfg:\r\n\tport_baud:%d,port_tx:%d,port_rx:%d,log_tx:%d,log_rx:%d\r\n",
           brd_cfg->brd_soft_cfg.cfg.port_baud, port_tx, port_rx, log_tx, log_rx);

    msleep(2);

    fpioa_set_function(port_tx, FUNC_UART1_TX + PROTOCOL_UART_NUM * 2);
    fpioa_set_function(port_rx, FUNC_UART1_RX + PROTOCOL_UART_NUM * 2);

    fpioa_set_function(log_tx, FUNC_UART1_TX + DBG_UART_NUM * 2);
    fpioa_set_function(log_rx, FUNC_UART1_RX + DBG_UART_NUM * 2);
}

static void init_relay_key_pin(board_cfg_t *brd_cfg)
{
    uint8_t key_dir, key_pin, relay_0_pin, relay_1_pin;

    key_dir = brd_cfg->brd_hard_cfg.uart_relay_key.key_dir;
    key_pin = brd_cfg->brd_hard_cfg.uart_relay_key.key;
    relay_0_pin = brd_cfg->brd_hard_cfg.uart_relay_key.relay_low;
    relay_1_pin = brd_cfg->brd_hard_cfg.uart_relay_key.relay_high;

    printk("board key relay cfg:\r\n\tkey:%d\tkey_dir:%d\trelay_high:%d\trelay_low:%d\r\n", key_pin, key_dir, relay_1_pin, relay_0_pin);

    //setup key low
    fpioa_set_function(relay_0_pin, FUNC_GPIOHS0 + CONFIG_RELAY_LOWX_GPIOHS_NUM);
    gpiohs_set_drive_mode(CONFIG_RELAY_LOWX_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_RELAY_LOWX_GPIOHS_NUM, 0);

    //setup key high
    fpioa_set_function(relay_1_pin, FUNC_GPIOHS0 + CONFIG_RELAY_HIGH_GPIOHS_NUM);
    gpiohs_set_drive_mode(CONFIG_RELAY_HIGH_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_RELAY_HIGH_GPIOHS_NUM, 1);

    //set up key
    fpioa_set_function(key_pin, FUNC_GPIOHS0 + CONFIG_FUNCTION_KEY_GPIOHS_NUM); //KEY
    if (key_dir)
    {
        gpiohs_set_drive_mode(CONFIG_FUNCTION_KEY_GPIOHS_NUM, GPIO_DM_INPUT_PULL_DOWN);
        gpiohs_set_pin_edge(CONFIG_FUNCTION_KEY_GPIOHS_NUM, GPIO_PE_RISING);
    }
    else
    {
        gpiohs_set_drive_mode(CONFIG_FUNCTION_KEY_GPIOHS_NUM, GPIO_DM_INPUT_PULL_UP);
        gpiohs_set_pin_edge(CONFIG_FUNCTION_KEY_GPIOHS_NUM, GPIO_PE_FALLING);
    }
    gpiohs_irq_register(CONFIG_FUNCTION_KEY_GPIOHS_NUM, 2, irq_gpiohs, NULL);

    sKey_dir = key_dir;

    return;
}

static void init_lcd_cam(board_cfg_t *brd_cfg)
{
    uint8_t lcd_dir, lcd_flip, lcd_hmirror, cam_flip, cam_hmirror;

    lcd_dir = brd_cfg->brd_hard_cfg.lcd_cam.lcd_dir;

    lcd_flip = brd_cfg->brd_hard_cfg.lcd_cam.lcd_flip;
    lcd_hmirror = brd_cfg->brd_hard_cfg.lcd_cam.lcd_hmirror;
    cam_flip = brd_cfg->brd_hard_cfg.lcd_cam.cam_flip;
    cam_hmirror = brd_cfg->brd_hard_cfg.lcd_cam.cam_hmirror;

    printk("board cfg lcd cam:\r\n\tlcd_flip:%d,lcd_hmirror:%d,cam_flip:%d,cam_hmirror:%d\r\n", lcd_flip, lcd_hmirror, cam_flip, cam_hmirror);

    lcd_dir |= (lcd_hmirror << 7) | (lcd_flip << 6);

    lcd_set_direction(lcd_dir);

    camera_set_hmirror(cam_hmirror);
    camera_set_vflip(cam_flip);

    // uint8_t new_reg[][2] = {
    //     {0x17, 0x14},
    //     {0, 0},
    // };

    // camera_modify_reg(new_reg);

#if CONFIG_CAMERA_GC0328_DUAL
    /* FIXME: face_lib have a BUG!!!*/
    camera_init(CAM_GC0328_DUAL);
#endif /* CONFIG_CAMERA_GC0328_DUAL */

    return;
}

//init uart port
int8_t protocol_init_device(board_cfg_t *brd_cfg, uint8_t op)
{
    /*load cfg from flash end*/
    init_relay_key_pin(brd_cfg);
    init_lcd_cam(brd_cfg);
    if (op)
    {
        //no uart
        return;
    }
    init_board_uart_port(brd_cfg);

    if (brd_cfg->brd_soft_cfg.cfg.pkt_fix)
    {
        g_pkt_head.pkt_json_start_1 = 0xaa;
        g_pkt_head.pkt_json_start_2 = 0x55;

        g_pkt_head.pkt_json_end_1 = 0x55;
        g_pkt_head.pkt_json_end_2 = 0xaa;

        g_pkt_head.pkt_jpeg_start_1 = 0xaa;
        g_pkt_head.pkt_jpeg_start_2 = 0x55;

        g_pkt_head.pkt_jpeg_end_1 = 0x55;
        g_pkt_head.pkt_jpeg_end_2 = 0xaa;
    }
    else
    {
        g_pkt_head.pkt_json_start_1 = 0; //start no sense
        g_pkt_head.pkt_json_start_2 = 0;

        g_pkt_head.pkt_json_end_1 = 0x0d;
        g_pkt_head.pkt_json_end_2 = 0x0a;

        g_pkt_head.pkt_jpeg_start_1 = 0xff;
        g_pkt_head.pkt_jpeg_start_2 = 0xd8;

        g_pkt_head.pkt_jpeg_end_1 = 0xff;
        g_pkt_head.pkt_jpeg_end_2 = 0xd9;
    }

    /* configure uart port  */
    memset(cJSON_prase_buf, 0, PROTOCOL_BUF_LEN);
    memset(cJSON_recv_buf, 0, PROTOCOL_BUF_LEN);

    uart_config(PROTOCOL_UART_NUM, brd_cfg->brd_soft_cfg.cfg.port_baud, 8, UART_STOP_1, UART_PARITY_NONE);
    uart_set_receive_trigger(PROTOCOL_UART_NUM, UART_RECEIVE_FIFO_1);
    uart_irq_register(PROTOCOL_UART_NUM, UART_RECEIVE, protocol_port_recv_cb, NULL, 3);

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void protocol_start_recv_jpeg(void)
{
    printk("protocol_start_recv_jpeg\r\n");

    jpeg_recv_start_flag = 1;
    recv_jpeg_flag = 1;
    jpeg_recv_len = 0;

    cal_pic_cfg.jpeg_buf = jpeg_recv_buf;

    jpeg_recv_start_time = sysctl_get_time_us();

    dvp_set_output_enable(0, 0);
    dvp_set_output_enable(1, 0);

    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
}

void protocol_stop_recv_jpeg(void)
{
    printk("protocol_stop_recv_jpeg\r\n");

    jpeg_recv_start_flag = 0;
    recv_jpeg_flag = 0;

    jpeg_recv_len = 0;
    jpeg_recv_start_time = 0;

    cal_pic_cfg.jpeg_buf = NULL;

    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);

    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern volatile uart_t *const uart[3];
static uint8_t uart_channel_getchar(uart_device_number_t channel, uint8_t *data)
{
    /* If received empty */
    if (!(uart[channel]->LSR & 1))
    {
        return 0;
    }
    *data = (uint8_t)(uart[channel]->RBR & 0xff);
    return 1;
}
