#ifndef __FACE_LIB_H
#define __FACE_LIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <float.h>
#include <math.h>
#include "fmath.h"

#include <bsp.h>
#include "atomic.h"
#include "platform.h"
#include "syscalls.h"
#include "utils.h"

#include "bsp.h"
#include "dmac.h"
#include "dvp.h"
#include "flash.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "i2c.h"
#include "plic.h"
#include "printf.h"
#include "sysctl.h"

#include "base64.h"
#include "cJSON.h"
#include "picojpeg.h"
#include "picojpeg_util.h"
#include "sha256.h"

#include "board.h"

#include "system_config.h"

/* clang-format off */
#define DBG_TIME_INIT()
#define DBG_TIME()

#define FEATURE_DIMENSION                   (196UL)
#define FACE_MAX_NUMBER                     (10UL)
#define FACE_RECGONITION_THRESHOLD          (80.0f)
#define FACE_RECGONITION_SCORE_MAX          (95.0f)

/* clang-format on */

typedef struct
{
    uint32_t width;
    uint32_t height;
    struct
    {
        uint32_t x;
        uint32_t y;
    } point[5];
} key_point_t;

typedef struct
{
    uint8_t *addr;
    uint16_t width;
    uint16_t height;
    uint16_t pixel;
} image_t;

typedef enum face_step
{
    STEP_DET_FACE,
    STEP_DET_KP,
    STEP_FETURE,
} face_step_t;

typedef struct
{
    uint32_t x1;
    uint32_t y1;
    uint32_t x2;
    uint32_t y2;
    uint32_t class_id;
    float prob;
    key_point_t key_point;
    float *feature;
    uint32_t index;
    float score;
    bool pass;
} face_obj_t;

typedef struct
{
    uint32_t obj_number;
    face_obj_t obj[FACE_MAX_NUMBER];
} face_obj_info_t;

typedef struct
{
    uint32_t result_number;
    face_obj_t *obj[FACE_MAX_NUMBER];
} face_compare_info_t;

typedef struct
{
    face_obj_info_t face_obj_info;
    face_compare_info_t face_compare_info;
} face_recognition_result_t;

typedef struct
{
    int ret;
    face_recognition_result_t *result;
} face_recognition_ret_t;

typedef struct
{
    uint8_t check_ir_face; //1 check, 0 not check
    uint8_t auto_out_fea;  //1 yes, 0 no

    float detect_threshold;
    float compare_threshold;
} face_recognition_cfg_t;

typedef struct
{
    //drae face edge on lcd image buf
    void (*lcd_draw_edge)(face_obj_t *obj, uint32_t color);

    //darw lcd image buf to lcd
    void (*lcd_draw_picture)(void);

    //draw not pass face on lcd
    void (*lcd_draw_false_face)(face_recognition_ret_t *ret);

    //save face to flash
    void (*record_face)(face_recognition_ret_t *ret);

    //face check pass callback
    void (*face_pass_cb)(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time);
} face_lib_callback_t;

///////////////////////////////////////////////////////////////////////////////
extern volatile uint8_t face_lib_draw_flag;
///////////////////////////////////////////////////////////////////////////////
//init module
uint8_t face_lib_init_module(void);

uint8_t face_lib_regisiter_callback(face_lib_callback_t *cfg_cb);

//cal camera pic face feature
void face_lib_run(face_recognition_cfg_t *cfg);

///////////////////////////////////////////////////////////////////////////////
/* uart protocol */
/* clang-format off */
#define IR_BOARD                (1)
#define PROTOCOL_BUF_LEN        (3 * 1024)
#define JPEG_BUF_LEN            (30 * 1024) //jpeg max 30K
/* clang-format on */

typedef struct _pkt_head
{
    uint8_t pkt_json_start_1;
    uint8_t pkt_json_start_2;

    uint8_t pkt_json_end_1;
    uint8_t pkt_json_end_2;

    uint8_t pkt_jpeg_start_1;
    uint8_t pkt_jpeg_start_2;

    uint8_t pkt_jpeg_end_1;
    uint8_t pkt_jpeg_end_2;

} pkt_head_t;

#include "uart_recv.h"

uint8_t protocol_send_init_done(void);

void protocol_prase(unsigned char *protocol_buf);
void protocol_cal_pic_fea(uint8_t *jpeg_buf, uint32_t jpeg_len);
uint8_t protocol_send_cal_pic_result(uint8_t code, char *msg, float feature[FEATURE_DIMENSION], uint8_t *uid, float prob);

uint8_t protocol_send_face_info(face_obj_t *obj,
                                float score, uint8_t uid[UID_LEN], float feature[FEATURE_DIMENSION],
                                uint32_t total, uint32_t current, uint64_t *time);
///////////////////////////////////////////////////////////////////////////////
/* w25qxx */
typedef enum _w25qxx_status
{
    W25QXX_OK = 0,
    W25QXX_BUSY,
    W25QXX_ERROR,
} w25qxx_status_t;

typedef enum _w25qxx_read
{
    W25QXX_STANDARD = 0,
    W25QXX_STANDARD_FAST,
    W25QXX_DUAL,
    W25QXX_DUAL_FAST,
    W25QXX_QUAD,
    W25QXX_QUAD_FAST,
} w25qxx_read_t;

w25qxx_status_t w25qxx_init(uint8_t spi_index, uint8_t spi_ss, uint32_t rate);
w25qxx_status_t w25qxx_read_id(uint8_t *manuf_id, uint8_t *device_id);
w25qxx_status_t w25qxx_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t my_w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, w25qxx_read_t mode);

///////////////////////////////////////////////////////////////////////////////
/* camera */
int dvp_irq(void *ctx);
int gc0328_init(uint16_t time); //sleep time in ms
void open_gc0328_650();
extern volatile uint8_t g_dvp_finish_flag;
///////////////////////////////////////////////////////////////////////////////

#endif
