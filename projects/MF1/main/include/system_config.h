#ifndef _SYSTEM_CONFIG_H
#define _SYSTEM_CONFIG_H

#include "uart.h"
#include "global_config.h"

/* clang-format off */

#define CONFIG_KEY_DIR                      (1)
#define CONFIG_KEY_LONG_RESTORE             (1)

#if CONFIG_WIFI_ENABLE
#define CONFIG_KEY_SHORT_QRCODE             (1)

#define CONFIG_PROTO_OVER_NET               (0)

#define CONFIG_NET_DEMO_MQTT                (1)
#define CONFIG_NET_DEMO_HTTP_GET            (1)
#define CONFIG_NET_DEMO_HTTP_POST           (1)
#else
#define CONFIG_KEY_SHORT_QRCODE             (0)

#define CONFIG_PROTO_OVER_NET               (0)

#define CONFIG_NET_DEMO_MQTT                (0)
#define CONFIG_NET_DEMO_HTTP_GET            (0)
#define CONFIG_NET_DEMO_HTTP_POST           (0)
#endif

///////////////////////////////////////////////////////////////////////////////
#define DBG_UART_NUM                   	    (UART_DEV3) //maybe bug
#define PROTOCOL_UART_NUM                   (UART_DEV1)

#if CONFIG_PROTOCOL_DEBUG_UART_PIN_SWAP
    #define PROTOCOL_PORT_TX_PIN                (CONFIG_DEBUG_UART_PORT_TX_PIN)
    #define PROTOCOL_PORT_RX_PIN                (CONFIG_DEBUG_UART_PORT_RX_PIN)
    #define DEBUE_TX_PIN                        (CONFIG_PROTOCOL_UART_PORT_TX_PIN)
    #define DEBUE_RX_PIN                        (CONFIG_PROTOCOL_UART_PORT_RX_PIN)
#else
    #define PROTOCOL_PORT_TX_PIN                (CONFIG_PROTOCOL_UART_PORT_TX_PIN)
    #define PROTOCOL_PORT_RX_PIN                (CONFIG_PROTOCOL_UART_PORT_RX_PIN)
    #define DEBUE_TX_PIN                        (CONFIG_DEBUG_UART_PORT_TX_PIN)
    #define DEBUE_RX_PIN                        (CONFIG_DEBUG_UART_PORT_RX_PIN)
#endif
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

#if 1 /* MF1 */
///////////////////////////////////////////////////////////////////////////////
/* PIN MAP */



/* WIFI */
#define WIFI_TX_PIN                         CONFIG_WIFI_PIN_TX
#define WIFI_RX_PIN                         CONFIG_WIFI_PIN_RX
#define WIFI_EN_PIN                         CONFIG_WIFI_PIN_ENABLE

#if CONFIG_WIFI_ENABLE
#define WIFI_SPI_SCLK_PIN                   WIFI_PIN_SPI_SCLK
#define WIFI_SPI_MOSI_PIN                   WIFI_PIN_SPI_MOSI
#define WIFI_SPI_MISO_PIN                   WIFI_PIN_SPI_MISO
#define WIFI_SPI_CSXX_PIN                   WIFI_PIN_SPI_CS
#endif

/* RGB LED */
#define RGB_LED_R_PIN                       CONFIG_LED_R_PIN
#define RGB_LED_G_PIN                       CONFIG_LED_G_PIN
#define RGB_LED_B_PIN                       CONFIG_LED_B_PIN


/* CAMERA */
#define CAM_SCL_PIN                         (41)
#define CAM_SDA_650_PIN                     (42)
#define CAM_SDA_850_PIN                     (40)

/* RELAY */
#define RELAY_LOWX_PIN                      CONFIG_RELAY_LOWX_PIN
#define RELAY_HIGH_PIN                      CONFIG_RELAY_HIGH_PIN


///////////////////////////////////////////////////////////////////////////////
/* FUNC MAP */

/* KEY */
#define KEY_HS_NUM                          CONFIG_FUNCTION_KEY_GPIOHS_NUM

/* IR LED */
#define IR_LED_HS_NUM                       CONFIG_INFRARED_GPIOHS_NUM

/* RGB LED */
#define RGB_LED_R_HS_NUM                    CONFIG_LED_R_GPIOHS_NUM
#define RGB_LED_G_HS_NUM                    CONFIG_LED_G_GPIOHS_NUM
#define RGB_LED_B_HS_NUM                    CONFIG_LED_B_GPIOHS_NUM


/* RELAY */
#define RELAY_LOWX_HS_NUM                   CONFIG_RELAY_LOWX_GPIOHS_NUM
#define RELAY_HIGH_HS_NUM                   CONFIG_RELAY_HIGH_GPIOHS_NUM

#if CONFIG_RELAY_LOWX_OPEN
    #define RELAY_LOWX_OPEN                     (1)
    #define RELAY_HIGH_OPEN                     (0)
#else
    #define RELAY_LOWX_OPEN                     (0)
    #define RELAY_HIGH_OPEN                     (1)
#endif
/* WIFI */
#define WIFI_TX_IO_NUM                      CONFIG_WIFI_GPIO_NUM_UART_TX  //GPIO
#define WIFI_RX_IO_NUM                      CONFIG_WIFI_GPIO_NUM_UART_RX  //GPIO
#define WIFI_EN_HS_NUM                      CONFIG_WIFI_GPIOHS_NUM_ENABLE

#if CONFIG_WIFI_ENABLE
#define WIFI_SPI_SS_HS_NUM                  CONFIG_WIFI_GPIOHS_NUM_CS
#endif

///////////////////////////////////////////////////////////////////////////////
#endif

//Note: Address must 4KB align
///////////////////////////////////////////////////////////////////////////////
#define BOARD_CFG_ADDR                              (0x7FF000)  //8M-4K
#define BOARD_CFG_LEN                               (4 * 1024)
#define WATCH_DOG_TIMEOUT                           (20000) //ms

//Face log	2M~3M
#define DATA_ADDRESS                                (0x800000)  //size=0x1000, 4KB,32768人
#define UID_LEN                                     (16)
#define NAME_LEN                                    (16)
#define NOTE_LEN                                    (16)
// #define UID_TABLE_SIZE	                            (0x4000)    //实际预留4096人，即64KB
#define FACE_DATA_ADDERSS                           (0x810000)  //8M+64K
#define FACE_HEADER                                 (0x55AA5503)
#define FACE_DATA_MAX_COUNT                         (512)
///////////////////////////////////////////////////////////////////////////////
//UI IMAGE	reserve 192KB for potential QVGA rgb565
#define IMG_LCD_SIZE		                        (240*240*2)
#define IMG_RECORD_FACE_ADDR                        (0xC00000)
#define IMG_FACE_PASS_ADDR                          (IMG_RECORD_FACE_ADDR + (116 * 1024 * 1))
#define IMG_CONNING_ADDR                            (IMG_RECORD_FACE_ADDR + (116 * 1024 * 2))
#define IMG_CONN_FAILED_ADDR                        (IMG_RECORD_FACE_ADDR + (116 * 1024 * 3))
#define IMG_CONN_SUCC_ADDR                          (IMG_RECORD_FACE_ADDR + (116 * 1024 * 4))
#define IMG_SCAN_QR_ADDR                            (IMG_RECORD_FACE_ADDR + (116 * 1024 * 5))
#define IMG_QR_TIMEOUT_ADDR                         (IMG_RECORD_FACE_ADDR + (116 * 1024 * 6))
///////////////////////////////////////////////////////////////////////////////
// AUDIO DATA
#define AUDIO_DATA_ADDR                             (0xE00000) 


/* clang-format on */

#endif
