#ifndef _SYSTEM_CONFIG_H
#define _SYSTEM_CONFIG_H

#include "uart.h"

/* clang-format off */

/* Camera Output */
#define IMG_W                               (320)
#define IMG_H                               (240)

///////////////////////////////////////////////////////////////////////////////
#define CONFIG_LCD_TYPE_ST7789              (1)
#define CONFIG_LCD_TYPE_SSD1963             (0)
#define CONFIG_LCD_TYPE_SIPEED              (0)

//START
#define CONFIG_SWAP_UART                    (1)//must be L18
#define CONFIG_KEY_LONG_CLEAR_FEA           (0)//must be L19
//END

#define CONFIG_ENABLE_WIFI                  (0)

#define CONFIG_KEY_DIR                      (1)
#define CONFIG_KEY_LONG_RESTORE             (1)

#if CONFIG_ENABLE_WIFI
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

#if (CONFIG_SWAP_UART==0)
#define PROTOCOL_PORT_TX_PIN                (10)
#define PROTOCOL_PORT_RX_PIN                (11)

#define DEBUE_TX_PIN                        (5)
#define DEBUE_RX_PIN                        (4)
#else
#define PROTOCOL_PORT_TX_PIN                (5)
#define PROTOCOL_PORT_RX_PIN                (4)

#define DEBUE_TX_PIN                        (10)
#define DEBUE_RX_PIN                        (11)
#endif
///////////////////////////////////////////////////////////////////////////////
#if CONFIG_LCD_TYPE_ST7789
#define LCD_240240                           (1)
#define LCD_320240                           (0)
#define LCD_ROTATE                           (1)

#define LCD_W                                (240)
#define LCD_H                                (240)
#elif CONFIG_LCD_TYPE_SSD1963
#define LCD_240240                            ()
#define LCD_320240                            ()
#define LCD_ROTATE                            ()

#define LCD_W                                 ()
#define LCD_H                                 ()
#elif CONFIG_LCD_TYPE_SIPEED
#define LCD_W                                (402)
#define LCD_H                                (240)

#define DAT_W                                (240)
#define DAT_H                                (240)
#endif

#define LCD_OFT                               ((IMG_W - LCD_W) / 2)
///////////////////////////////////////////////////////////////////////////////

#if 1 /* MF1 */
///////////////////////////////////////////////////////////////////////////////
/* PIN MAP */

/* KEY */
#define KEY_PIN                             (24)

/* IR LED */
#define IR_LED_PIN                          (32)

/* LCD_BL */
#define LCD_BL_PIN                          (9)

/* WIFI */
#define WIFI_TX_PIN                         (6)
#define WIFI_RX_PIN                         (7)
#define WIFI_EN_PIN                         (8)

#if CONFIG_ENABLE_WIFI
#define WIFI_SPI_SCLK_PIN                   (1)
#define WIFI_SPI_MOSI_PIN                   (3)
#define WIFI_SPI_MISO_PIN                   (2)
#define WIFI_SPI_CSXX_PIN                   (0)
#endif

/* RGB LED */
#define RGB_LED_R_PIN                       (21)
#define RGB_LED_G_PIN                       (22)
#define RGB_LED_B_PIN                       (23)

/* LCD */
#define LCD_RST_PIN                         (37)
#define LCD_DCX_PIN                         (38)
#define LCD_WRX_PIN                         (36)
#define LCD_SCK_PIN                         (39)

/* CAMERA */
#define CAM_SCL_PIN                         (41)
#define CAM_SDA_650_PIN                     (42)
#define CAM_SDA_850_PIN                     (40)

/* RELAY */
#define RELAY_LOWX_PIN                      (12)
#define RELAY_HIGH_PIN                      (13)

/* TF CARD */
#define TF_SPI_SCLK_PIN                     (27)
#define TF_SPI_MOSI_PIN                     (28)
#define TF_SPI_MISO_PIN                     (26)
#define TF_SPI_CSXX_PIN                     (29)
///////////////////////////////////////////////////////////////////////////////
/* FUNC MAP */

/* KEY */
#define KEY_HS_NUM                          (0)

/* IR LED */
#define IR_LED_HS_NUM                       (1)

/* RGB LED */
#define RGB_LED_R_HS_NUM                    (2)
#define RGB_LED_G_HS_NUM                    (3)
#define RGB_LED_B_HS_NUM                    (4)

/* LCD */
#define LCD_DCX_HS_NUM                      (5)
#define LCD_RST_HS_NUM                      (6)

/* RELAY */
#define RELAY_LOWX_HS_NUM                   (7)
#define RELAY_HIGH_HS_NUM                   (8)

#define RELAY_LOWX_OPEN                     (1)
#define RELAY_HIGH_OPEN                     (0)

/* WIFI */
#define WIFI_TX_IO_NUM                      (0)  //GPIO
#define WIFI_RX_IO_NUM                      (1)  //GPIO
#define WIFI_EN_HS_NUM                      (10)

#if CONFIG_ENABLE_WIFI
#define WIFI_SPI_SS_HS_NUM                  (11)
#endif

/* TF CARD */
#define TF_SPI_SCLK_HS_NUM                  (12)
#define TF_SPI_MOSI_HS_NUM                  (13)
#define TF_SPI_MISO_HS_NUM                  (14)
#define TF_SPI_CSXX_HS_NUM                  (15)
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
/* clang-format on */

#endif
