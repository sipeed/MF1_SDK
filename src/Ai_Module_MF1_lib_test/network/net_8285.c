#include "net_8285.h"

#include "board.h"

#include "cJSON.h"
#include "qrcode.h"

//spi wifi
#include "WiFiSpi.h"
#include "myspi.h"

#include "system_config.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* for qrcode scan */
volatile uint8_t g_net_status = 0;
volatile uint8_t qrcode_get_info_flag = 0;

uint64_t qrcode_start_time = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* for mqtt */
static unsigned char *id_fmt = "%02X:%02X:%02X:%02X:%02X:%02X";
static unsigned char mqttc_id[32];

static uint8_t *mqtt_ip = NULL;
static unsigned char *mqtt_domain = "mq.tongxinmao.com";
static uint16_t mqtt_port = 18830;

static unsigned char *mqtt_topic = "/public/TEST/SIPEED";
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//0 ok
//1 json prase failed
//2 ssid or mac or passwd len error
//3 mac not for this borad
static uint8_t protocol_prase_qrcode_get_wifi_info(char *msg_buf, uint8_t ssid[32], uint8_t passwd[32])
{
    uint8_t err = 0, w_len, mac[20];

    cJSON *root = NULL, *tmp = NULL;

    root = cJSON_Parse(msg_buf);
    if(root == NULL)
    {
        printf("prase qrcode payload failed\r\n");
        err = 1;
        goto _prase_json_error;
    }
    //get ssid
    tmp = cJSON_GetObjectItem(root, "w");
    if(tmp == NULL)
    {
        printf("get \"w\"failed\r\n");
        err = 1;
        goto _prase_json_error;
    }
    w_len = strlen(tmp->valuestring);
    if(w_len > 32)
    {
        printf("ssid len too long\r\n");
        err = 2;
        goto _prase_json_error;
    }
    memcpy(ssid, tmp->valuestring, w_len);
    ssid[w_len] = 0;
    printf("qrcode ssid:%s\r\n", ssid);

    //get passwd
    tmp = cJSON_GetObjectItem(root, "p");
    if(tmp == NULL)
    {
        printf("get \"p\"failed\r\n");
        err = 1;
        goto _prase_json_error;
    }
    w_len = strlen(tmp->valuestring);
    if(w_len > 32 || w_len < 8)
    {
        printf("password len too long\r\n");
        err = 2;
        goto _prase_json_error;
    }
    memcpy(passwd, tmp->valuestring, w_len);
    passwd[w_len] = 0;
    printf("qrcode passwd:%s\r\n", passwd);

    //get mac
    tmp = cJSON_GetObjectItem(root, "t");
    if(tmp == NULL)
    {
        printf("get \"t\"failed\r\n");
        err = 1;
        goto _prase_json_error;
    }
    w_len = strlen(tmp->valuestring);
    printf("mac len:%d\r\n", w_len);
    if(w_len > 21)
    {
        printf("mac len too long\r\n");
        err = 2;
        goto _prase_json_error;
    }
    memcpy(mac, tmp->valuestring, w_len);
    mac[w_len] = 0;
    printf("qrcode mac:%s\r\n", mac);

    if(memcmp(mac, mqttc_id, w_len) != 0)
    {
        printf("mac error, not for my qrcoder\n");
        err = 3;
        goto _prase_json_error;
    }
_prase_json_error:
    if(root)
        cJSON_Delete(root);
    return err;
}

/* {"t":"84:0D:8E:6C:62:9C","w":"Sipeed_2.4G","p":"Sipeed123."} */
qr_wifi_info_t *qrcode_get_wifi_cfg(void)
{
    uint8_t scan_ret = -1;
    qrcode_image_t img;
    qrcode_result_t qrcode;

    qr_wifi_info_t *ret = (qr_wifi_info_t *)malloc(sizeof(qr_wifi_info_t));
    if(ret == NULL)
    {
        return NULL;
    }

    if((sysctl_get_time_us() - qrcode_start_time) > 10 * 1000 * 1000)
    {
        ret->ret = QRCODE_RET_CODE_TIMEOUT;
        return ret;
    }

    memset(&img, 0, sizeof(img));
    img.w = IMG_W; /* 320 */
    img.h = IMG_H; /* 240 */
    img.data = (uint8_t *)display_image;

    ret->ret = QRCODE_RET_CODE_NO_DATA;
    if(find_qrcodes(&qrcode, &img) != 0)
    {
        if(/* qrcode.version <= 6 && */ qrcode.payload_len <= 110)
        {
            // printf("payload:%s\r\n", qrcode.payload);
            scan_ret = protocol_prase_qrcode_get_wifi_info(qrcode.payload,
                                                           ret->ssid, ret->passwd);

            if(scan_ret == 0)
            {
                printf("get cfg success\r\n");
                ret->ret = QRCODE_RET_CODE_OK;
            }
        } else
        {
            printf("qrcode type error\r\n");
            ret->ret = QRCODE_RET_CODE_PRASE_ERR;
        }
    }
    return ret;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//0 ok
//1 module init failed
//2 module communication error
uint8_t spi_8266_init_device(void)
{
    //not connect to network
    g_net_status = 0;
    //hard reset
    gpiohs_set_pin(WIFI_EN_HS_NUM, 0); //disable WIFI
    msleep(50);
    gpiohs_set_pin(WIFI_EN_HS_NUM, 1); //enable WIFI
    msleep(500);

    my_spi_init();
    WiFiSpi_init(-1);

    // check for the presence of the ESP module:
    if(WiFiSpi_status() == WL_NO_SHIELD)
    {
        printf("WiFi module not present\r\n");
        // don't continue:
        return 1;
    }

    // WiFiSpi_softReset();

    printf("firmware version:%s\r\n", WiFiSpi_firmwareVersion());
    printf("protocol version : %s\r\n", WiFiSpi_protocolVersion());
    printf("status:%d\r\n", WiFiSpi_status());

    if(!WiFiSpi_checkProtocolVersion())
    {
        printf("Protocol version mismatch. Please upgrade the firmware\r\n");
        // don't continue:
        return 2;
    }

#if 0
    uint8_t mac[6];
    WiFiSpi_macAddress(mac);
#else
    uint8_t mac[6] = {0xDC, 0x4F, 0x22, 0x50, 0x97, 0xC3};
#endif

    sprintf(mqttc_id, id_fmt, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("mqtt_client_id: %s\r\n", mqttc_id);

    return 0;
}

//1 connect ok
//0 connect timeout
uint8_t spi_8266_connect_ap(uint8_t wifi_ssid[32],
                            uint8_t wifi_passwd[32],
                            uint8_t timeout_n10s)
{
    uint8_t retry_cnt = 0;
    uint8_t status = WL_IDLE_STATUS; // the Wifi radio's status

    g_net_status = 0;

    while(status != WL_CONNECTED)
    {
        if(retry_cnt >= timeout_n10s)
        {
            return 0;
        }
        printf("Attempting to connect to WPA SSID: %s\r\n", wifi_ssid);
        status = WiFiSpi_begin_ssid_passwd(wifi_ssid, wifi_passwd);
        if(status == WL_CONNECTED)
            goto connect_success;
        retry_cnt++;
    }
connect_success:
    printf("You're connected to the network\r\n");
    g_net_status = 1;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback function
static uint16_t last_msgId = -1;
static void mqtt_callback(unsigned char *intopic, uint16_t msgId, unsigned char *payload, unsigned int length)
{
    *(payload + length) = 0;

#if 1
    printf("topic: %s \r\n", intopic);
    printf("msgId: %d \r\n", msgId);
    printf("payload len: %d \r\n", length);
    printf("payload: %s\r\n\r\n", payload);
#endif

    if(last_msgId != msgId)
    {
        last_msgId = msgId;
        /* TODO: here you can prase mqtt msg */
    } else
    {
        printf("same msgId\r\n");
    }
}

//FIXME: 如果服务器掉线，不能一直等待。。。
void mqtt_reconnect(void)
{
    while(!PubSubClient_connected())
    {
        printf("Attempting MQTT connection...\r\n");
        if(PubSubClient_connect1(mqttc_id, NULL, NULL))
        {
            printf("reconnected\r\n");
            PubSubClient_publish(mqtt_topic, "reconnected", strlen("reconnected"));
            PubSubClient_subscribe(mqtt_topic, 1);
        } else
        {
            printf("failed, rc=%d\r\n", PubSubClient_state());
        }
    }
}

uint8_t spi_8266_mqtt_init(void)
{
#if 0
    uint8_t mac[6];
    WiFiSpi_macAddress(mac);
#else
    uint8_t mac[6] = {0xDC, 0x4F, 0x22, 0x50, 0x97, 0xC3};
#endif

    sprintf(mqttc_id, id_fmt, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("Please see http://www.tongxinmao.com/txm/webmqtt.php\r\n");

    printf("mqtt_client_id: %s\r\n", mqttc_id);
    printf("mqtt_topic:%s\r\n", mqtt_topic);

    PubSubClient_init(mqtt_domain, mqtt_ip, mqtt_port, 0, 60, mqtt_callback);
    // Connect to mqtt broker
    if(PubSubClient_connect1(mqttc_id, NULL, NULL))
    {
        printf("mqtt connect successed\r\n");
        PubSubClient_publish(mqtt_topic, "hello world", strlen("hello world"));
        PubSubClient_subscribe(mqtt_topic, 1);
    } else
    {
        printf("mqtt connect failed\r\n");
    }
    return 1;
}
