#ifndef __NET_8285_H
#define __NET_8285_H

#include <stdint.h>
#include <stddef.h>

//mqtt
#include "PubSubClient.h"

/* clang-format off */
/* clang-format on */

enum
{
    QRCODE_RET_CODE_OK,        /* 0 */
    QRCODE_RET_CODE_NO_DATA,   /* 1 */
    QRCODE_RET_CODE_PRASE_ERR, /* 2 */
    QRCODE_RET_CODE_TIMEOUT    /* 3 */
};

typedef struct _qr_wifi_info
{
    uint8_t ret;

    char ssid[32];
    char passwd[32];
} qr_wifi_info_t;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern volatile uint8_t g_net_status;
extern volatile uint8_t qrcode_get_info_flag;
extern uint64_t qrcode_start_time;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

qr_wifi_info_t *qrcode_get_wifi_cfg(void);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t spi_8266_init_device(void);
uint8_t spi_8266_connect_ap(uint8_t wifi_ssid[32],
                            uint8_t wifi_passwd[32],
                            uint8_t timeout_n10s);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mqtt_reconnect(void);
uint8_t spi_8266_mqtt_init(void);
void spi_8266_mqtt_send(char *buf,size_t len);

#endif
