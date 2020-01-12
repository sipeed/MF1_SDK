#include "user_cmd.h"

#include "face_lib.h"
#include "face_cb.h"

#include "cJSON.h"
#include "maix_qrcode.h"

#include "board.h"

#if CONFIG_ENABLE_UART_PROTOCOL
///////////////////////////////////////////////////////////////////////////////
static void hex_str(uint8_t *inchar, uint16_t len, uint8_t *outtxt);
static uint16_t str_hex(uint8_t *str, uint8_t *hex);

static void proto_query_uid(cJSON *root);
static void proto_set_face_recognition_stat(cJSON *root);
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
    {.cmd = "query_uid", .cmd_cb = proto_query_uid},
    {.cmd = "face_recon", .cmd_cb = proto_set_face_recognition_stat},
    {.cmd = "qrscan", .cmd_cb = proto_scan_qrcode},
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* code:
        0: 有
        1: 没有 
        2: 解析错误
*/
static void proto_query_uid_ret(uint8_t code, char *msg, uint32_t uid_id)
{
    cJSON *ret = NULL;

    ret = protocol_gen_header("query_uid_ret", code, msg);

    if (ret == NULL)
    {
        printk("%s Ahh, failed to gen header\r\n", __func__);
        return 1;
    }

    cJSON_AddNumberToObject(ret, "uid_id", uid_id);

    protocol_send(ret);

    cJSON_Delete(ret);

    return 0;
}

/* 这里是去查询某一个UID是否存在 */
/* {"uid":"xxxxxxx"} */
static void proto_query_uid(cJSON *root)
{
    cJSON *tmp = NULL;
    uint32_t uid_id = 0;
    uint8_t hex_uid[16];

    tmp = cJSON_GetObjectItem(root, "uid");
    if ((tmp == NULL) || (cJSON_IsString(tmp) == false))
    {
        printk("get uid failed\r\n");
        proto_query_uid_ret(2, "no uid", 0);
        goto _exit;
    }

    if (str_hex(tmp->valuestring, hex_uid) != 16)
    {
        proto_query_uid_ret(2, "uid len error", 0); //json解析出错
        goto _exit;
    }

    uid_id = flash_get_id_by_uid(hex_uid);
    printk("\r\nuid_id:%08x\r\n", uid_id);

    if (uid_id != 0)
    {
        proto_query_uid_ret(0, "uid exist", uid_id & 0x0FFFFFFF);
    }
    else
    {
        proto_query_uid_ret(1, "uid not exist", 0);
    }
_exit:
    return;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* code:
        0: Ok
        1: 解析失败
*/
static void proto_set_face_recognition_stat_ret(uint8_t code, char *msg, uint8_t stat)
{
    cJSON *ret = NULL;

    ret = protocol_gen_header("face_recon_ret", code, msg);

    if (ret == NULL)
    {
        printk("%s Ahh, failed to gen header\r\n", __func__);
        return 1;
    }

    cJSON_AddNumberToObject(ret, "stat", stat);

    protocol_send(ret);

    cJSON_Delete(ret);

    return 0;
}

/* 默认使能识别,这个标志位影响几个回调，设置为0就不画人脸框，也不执行后续的回调 */
uint8_t proto_start_face_recon_flag = 1;

/* {"query_stat":0/1,"set_stat":0/1} */
static void proto_set_face_recognition_stat(cJSON *root)
{
    cJSON *tmp = NULL;

    tmp = cJSON_GetObjectItem(root, "query_stat");
    if ((tmp != NULL) && (cJSON_IsNumber(tmp) != false))
    {
        if ((tmp->valueint & 0x1) == 0x1)
        {
            printk("user query face recon stat\r\n");

            /* 返回当前状态 */
            proto_set_face_recognition_stat_ret(0, "query_stat success", proto_start_face_recon_flag);
            goto _exit;
        }
    }

    tmp = cJSON_GetObjectItem(root, "set_stat");
    if ((tmp != NULL) && (cJSON_IsNumber(tmp) != false))
    {
        printk("user set face recon stat %d to %d\r\n", proto_start_face_recon_flag, (tmp->valueint & 0x1));
        proto_start_face_recon_flag = (tmp->valueint & 0x1);
        proto_set_face_recognition_stat_ret(0, "set_stat success", proto_start_face_recon_flag);
        goto _exit;
    }
    /* 这里返回解析错误 */
    proto_set_face_recognition_stat_ret(1, "pkt parse failed", 0);
_exit:
    return;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
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
    cJSON *ret = NULL;

    ret = protocol_gen_header("set_notify_ret", code, msg);

    if (ret == NULL)
    {
        printk("%s Ahh, failed to gen header\r\n", __func__);
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
        proto_set_notify_ret(0, "save cfg success", g_board_cfg.user_custom_cfg[0], g_board_cfg.user_custom_cfg[1]);
        return;
    }
    printk("save flash cfg faild\r\n");
    proto_set_notify_ret(2, "save cfg failed", 0, 0);
    return;
}
#endif /* CONFIG_NOTIFY_STRANGER */
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
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
    cJSON *ret = NULL;

    /* 停止扫码 */
    proto_scan_qrcode_flag = 0;

    ret = protocol_gen_header("qrscan_ret", code, msg);

    if (ret == NULL)
    {
        printk("%s Ahh, failed to gen header\r\n", __func__);
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void hex_str(uint8_t *inchar, uint16_t len, uint8_t *outtxt)
{
    uint16_t i;
    uint8_t hbit, lbit;

    for (i = 0; i < len; i++)
    {
        hbit = (*(inchar + i) & 0xf0) >> 4;
        lbit = *(inchar + i) & 0x0f;
        if (hbit > 9)
            outtxt[2 * i] = 'A' + hbit - 10;
        else
            outtxt[2 * i] = '0' + hbit;
        if (lbit > 9)
            outtxt[2 * i + 1] = 'A' + lbit - 10;
        else
            outtxt[2 * i + 1] = '0' + lbit;
    }
    outtxt[2 * i] = 0;
    return;
}

static uint16_t str_hex(uint8_t *str, uint8_t *hex)
{
    uint8_t ctmp, ctmp1, half;
    uint16_t num = 0;
    do
    {
        do
        {
            half = 0;
            ctmp = *str;
            if (!ctmp)
                break;
            str++;
        } while ((ctmp == 0x20) || (ctmp == 0x2c) || (ctmp == '\t'));
        if (!ctmp)
            break;
        if (ctmp >= 'a')
            ctmp = ctmp - 'a' + 10;
        else if (ctmp >= 'A')
            ctmp = ctmp - 'A' + 10;
        else
            ctmp = ctmp - '0';
        ctmp = ctmp << 4;
        half = 1;
        ctmp1 = *str;
        if (!ctmp1)
            break;
        str++;
        if ((ctmp1 == 0x20) || (ctmp1 == 0x2c) || (ctmp1 == '\t'))
        {
            ctmp = ctmp >> 4;
            ctmp1 = 0;
        }
        else if (ctmp1 >= 'a')
            ctmp1 = ctmp1 - 'a' + 10;
        else if (ctmp1 >= 'A')
            ctmp1 = ctmp1 - 'A' + 10;
        else
            ctmp1 = ctmp1 - '0';
        ctmp += ctmp1;
        *hex = ctmp;
        hex++;
        num++;
    } while (1);
    if (half)
    {
        ctmp = ctmp >> 4;
        *hex = ctmp;
        num++;
    }
    return (num);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////