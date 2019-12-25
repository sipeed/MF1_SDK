#include "user_cmd.h"

#include "face_lib.h"
#include "face_cb.h"

#include "cJSON.h"
#include "qrcode.h"

#include "board.h"

#if CONFIG_ENABLE_UART_PROTOCOL
///////////////////////////////////////////////////////////////////////////////
static void test_cmd(cJSON *root);
static void test2_cmd(cJSON *root);
static void proto_scan_qrcode(cJSON *root);
static void proto_send_qrcode_ret(uint8_t code, char *msg, char *qrcode);

///////////////////////////////////////////////////////////////////////////////
/* 增删指令必须修改数组大小 */
#if CONFIG_NOTIFY_STRANGER
static void proto_set_notify(cJSON *root);

protocol_custom_cb_t user_custom_cmd[4] = {
    {.cmd = "set_notify", .cmd_cb = proto_set_notify},
#else
protocol_custom_cb_t user_custom_cmd[3] = {
#endif /* CONFIG_NOTIFY_STRANGER */
    {.cmd = "test", .cmd_cb = test_cmd},
    {.cmd = "test2", .cmd_cb = test2_cmd},
    {.cmd = "qrscan", .cmd_cb = proto_scan_qrcode},
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//recv: {"version":1,"type":"test"}
//send: {"version":1,"type":"test","code":0,"msg":"test"}
static void test_cmd(cJSON *root)
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
static void test2_cmd(cJSON *root)
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

///////////////////////////////////////////////////////////////////////////////
/* 对客户需求输出陌生人脸需求添加的指令 */
#if CONFIG_NOTIFY_STRANGER
/* code:
        0, success
        1, Json parse failed
        2, Unk error
*/
static uint8_t proto_set_notify_ret(uint8_t code, char *msg, uint8_t en, uint8_t out_fea)
{
    const char *notify_cmd_ret = "set_notify_ret";

    cJSON *ret = NULL;

    ret = protocol_gen_header(notify_cmd_ret, code, msg);

    if (ret == NULL)
    {
        printk("Ahh, failed to gen header\r\n");
        return 1;
    }

    cJSON_AddNumberToObject(ret, "en", en);
    cJSON_AddNumberToObject(ret, "out_fea", out_fea);

    protocol_send(ret);

    cJSON_Delete(ret);

    return 0;
}

static void proto_set_notify(cJSON *root)
{
    cJSON *tmp = NULL;
    uint8_t en, out_fea;

    tmp = cJSON_GetObjectItem(root, "query");
    if ((tmp == NULL) || cJSON_IsNumber(tmp) == false)
    {
        printk("can not get query\r\n");
        proto_set_notify_ret(1, "get query failed", 0, 0);
        return;
    }

    if ((tmp->valueint & 0x01) == 0x01)
    {
        proto_set_notify_ret(0, "query success", g_board_cfg.user_custom_cfg[0], g_board_cfg.user_custom_cfg[1]);
        return;
    }

    tmp = cJSON_GetObjectItem(root, "en");
    if ((tmp == NULL) || cJSON_IsNumber(tmp) == false)
    {
        printk("can not get en\r\n");
        proto_set_notify_ret(1, "get en failed", 0, 0);
        return;
    }
    en = tmp->valueint & 0x01;
    printk("set en %d\r\n", en);

    tmp = cJSON_GetObjectItem(root, "out_fea");
    if ((tmp == NULL) || cJSON_IsNumber(tmp) == false)
    {
        printk("can not get out_fea\r\n");
        proto_set_notify_ret(1, "get out_fea failed", 0, 0);
        return;
    }
    out_fea = tmp->valueint & 0x01;
    printk("set enout_fea %d\r\n", out_fea);

    g_board_cfg.user_custom_cfg[0] = en;
    g_board_cfg.user_custom_cfg[1] = out_fea;

    if (flash_save_cfg(&g_board_cfg))
    {
        printk("save flash cfg success\r\n");
        proto_set_notify_ret(0, "save cfg success", 0, 0);
        return;
    }
    printk("save flash cfg faild\r\n");
    proto_set_notify_ret(2, "save cfg failed", 0, 0);
    return;
}
#endif /* CONFIG_NOTIFY_STRANGER */
///////////////////////////////////////////////////////////////////////////////
int proto_scan_qrcode_flag = 0;

static qrcode_scan_t qrcode_cfg = {
    .scan_time_out_s = 0,
    .start_time_us = 0,
    .img_w = CONFIG_CAMERA_RESOLUTION_WIDTH,
    .img_h = CONFIG_CAMERA_RESOLUTION_HEIGHT,
    .img_data = NULL,
};

/* {"version":2,"type":"qrscan","time_out_s":2} */
/* {"version":2,"type":"qrscan_ret","code":0,"msg":"scan qrcode success","qrcode":"xxxx"} */
static void proto_scan_qrcode(cJSON *root)
{
    cJSON *tmp = NULL;
    int proto_scan_qrcode_timeout_s = 0;

    tmp = cJSON_GetObjectItem(root, "time_out_s");

    if ((tmp == NULL) || (cJSON_IsNumber(tmp) == false))
    {
        printk("get time_out_s failed\r\n");
        proto_send_qrcode_ret(1, "time_out_s error", NULL);
        return;
    }

    set_lcd_bl(1); //FIXME: 这个和lib是独立的,用户按键扫码会开背光,如果之后没有检测到人脸,那么背光就一直亮了,所以...

    set_IR_LED(0);
    set_W_LED(0);
    // dvp_set_output_enable(0, 0); //disable to ai
    // dvp_set_output_enable(1, 1); //enable to lcd

    proto_scan_qrcode_timeout_s = tmp->valueint;

    printk("scan qrcode time_out_s %d \r\n", proto_scan_qrcode_timeout_s);

    qrcode_cfg.scan_time_out_s = proto_scan_qrcode_timeout_s;
    qrcode_cfg.start_time_us = sysctl_get_time_us();

    proto_scan_qrcode_flag = 1;

    return;
}

/* code
        0: success
        1: failed to parse cmd
        2: timeout
        3: error
 */
static void proto_send_qrcode_ret(uint8_t code, char *msg, char *qrcode)
{
    const char *qrcode_cmd_ret = "qrscan_ret";
    cJSON *ret = NULL;

    /* 停止扫码 */
    proto_scan_qrcode_flag = 0;

    ret = protocol_gen_header(qrcode_cmd_ret, code, msg);

    if (ret == NULL)
    {
        printk("Ahh, failed to gen header\r\n");
        return;
    }

    if (qrcode)
    {
        cJSON_AddStringToObject(ret, "qrcode", qrcode);
    }
    else
    {
        cJSON_AddNullToObject(ret, "qrcode");
    }

    protocol_send(ret);

    cJSON_Delete(ret);

    return;
}

extern void rgb_lock_buf(void);
extern void rgb_unlock_buf(void);
extern void kpu_lock_buf(void);
extern void kpu_unlock_buf(void);

void proto_qrcode_scan_loop(void)
{
#if !CONFIG_ALWAYS_SCAN_QRCODES
    if (proto_scan_qrcode_flag)
    {
        kpu_lock_buf();
        rgb_lock_buf();
#endif /* CONFIG_ALWAYS_SCAN_QRCODES */
        uint64_t t = sysctl_get_time_us();

        qrcode_cfg.img_data = display_image;
        enum enum_qrcode_res ret = qrcode_scan(&qrcode_cfg, 1);

        printk("qrcode: %ld ms\r\n", (sysctl_get_time_us() - t) / 1000);

        switch (ret)
        {
        case QRCODE_SUCC:
        {
            printk("QRCODE_SUCC\r\n");
            proto_send_qrcode_ret(0, "success", qrcode_cfg.qrcode);
        }
        break;
        case QRCODE_TIMEOUT:
        {
            printk("QRCODE_TIMEOUT\r\n");
            proto_send_qrcode_ret(2, "timeout", NULL);
        }
        break;
        case QRCODE_ERROR:
        {
            printk("QRCODE_ERROR\r\n");
            proto_send_qrcode_ret(3, "qrcode error", NULL);
        }
        default:
        {
            printk("no qrcode\r\n");
#if !CONFIG_ALWAYS_SCAN_QRCODES
            printk("%s\r\n", (ret == QRCODE_NONE) ? "QRCODE_FAIL" : "QRCODE_UNK_ERR");
            /* here display pic */
            lcd_display_image_alpha(IMG_SCAN_QR_ADDR, 150);
#endif
        }
        break;
        }

#if !CONFIG_ALWAYS_SCAN_QRCODES
        rgb_unlock_buf();
        kpu_unlock_buf();
    }
#endif /* CONFIG_ALWAYS_SCAN_QRCODES */

    return;
}
///////////////////////////////////////////////////////////////////////////////
#endif /* CONFIG_ENABLE_UART_PROTOCOL */
