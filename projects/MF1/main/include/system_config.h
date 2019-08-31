#ifndef _SYSTEM_CONFIG_H
#define _SYSTEM_CONFIG_H

#include "uart.h"
#include "global_config.h"

/* clang-format off */

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

///////////////////////////////////////////////////////////////////////////////
/* CAMERA */
#define CAM_SCL_PIN                         (41)
#define CAM_SDA_650_PIN                     (42)
#define CAM_SDA_850_PIN                     (40)
///////////////////////////////////////////////////////////////////////////////

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
