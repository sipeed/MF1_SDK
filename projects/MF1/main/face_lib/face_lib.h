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

#include <bsp.h>
#include "atomic.h"
#include "platform.h"
#include "syscalls.h"
#include "utils.h"

#include "dmac.h"
#include "dvp.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "i2c.h"
#include "plic.h"
#include "printf.h"
#include "sha256.h"
#include "sysctl.h"

#include "base64.h"
#include "board.h"
#include "cJSON.h"
#include "camera.h"
#include "flash.h"
#include "lcd.h"
#include "lcd_dis.h"
#include "picojpeg.h"
#include "picojpeg_util.h"

#include "system_config.h"

/* clang-format off */

#define _IS_IOMEM(x) (!((uint64_t)(&(x))&0x80000000))
#define _IS_CACHEMEM(x) ((uint64_t)(&(x))&0x80000000)
#define _IS_IOMEMP(x) (!((uint64_t)(x)&0x80000000))
#define _IS_CACHEMEMP(x) ((uint64_t)(x)&0x80000000)

#define _IOMEM(x, type) (*(type *)(((uint64_t)&(x))-0x40000000))
#define _IOMEM_UINT8(x) (*(uint8_t *)(((uint64_t)&(x))-0x40000000))
#define _IOMEM_ADDR(x) (_IS_IOMEM(x)?(uint64_t)(x):(((uint64_t)&(x))-0x40000000))
#define _IOMEM_PADDR(p) (_IS_IOMEMP(p)?(uint64_t)(p):(((uint64_t)(p))-0x40000000))
#define _ADDR(x)	((uint64_t)(x))
#define _IN_BUF(x, buf)	(_ADDR(x)>=_ADDR(buf) && _ADDR(x)<_ADDR(buf)+sizeof(buf))
#define CHECK_IOMEM(x) configASSERT(_IS_IOMEM(x))
#define CHECK_IOMEMP(x) configASSERT(_IS_IOMEMP(x))

#define _CACHE_ADDR(x)                          (_IS_CACHEMEM(x)?(uint64_t)(x):(((uint64_t)&(x))+0x40000000))
#define _CACHE_PADDR(p)                         (_IS_CACHEMEMP(p)?(uint64_t)(p):(((uint64_t)(p))+0x40000000))


#define _MODULE_ADDR(x) (x)

#define FTR_850                             (1)
#define FTR_650                             (0)

#define FEATURE_DIMENSION                   (196UL)
#define FACE_MAX_NUMBER                     (10UL)
#define FACE_RECGONITION_THRESHOLD          (77.0f)
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
    int8_t *feature;
    uint32_t index;
    float score;
    bool pass;
    int blur;
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
    uint8_t check_ir_face; //1 check, 0 not check, not support now
    uint8_t auto_out_fea;  //1 yes, 0 no

    uint8_t night_threshold; //camera 650, judge night threshold,default 60

    uint8_t use_flash_led;     //1 when dark, use flash led
    uint8_t no_face_close_lcd; //1 enable, 0 disable

    int face_min_width;
    int face_min_height;
    
    float detect_threshold;
    float compare_threshold;
} face_recognition_cfg_t;

/*
    detected_face_cb: 检测到人脸的回调，即原lcd_draw_false_face的位置  record_face_cb: 录入人脸的回调
    fake_face_cb：检测到已录入的，但是活体检测不过的人脸（也可能是已录入但角度不对的）
    pass_face_cb：检测到已录入且通过活体（如果设置了需要活体的话）的回调，即原来的pass回调
    lcd_refresh_cb: 刷屏回调
 */

typedef uint8_t (*add_spec_uid_user_ret)(uint8_t code, char *msg, uint8_t *uid);

typedef struct
{
    uint8_t uid[16];
    uint8_t time_out_s;

    add_spec_uid_user_ret send_ret;

} proto_record_face_cfg_t;

typedef struct
{
    //protocol send
    void (*proto_send)(char *buf, size_t len);

    void (*proto_record_face)(proto_record_face_cfg_t *cfg);

    //detect face, user can record face
    void (*detected_face_cb)(face_recognition_ret_t *face);

    void (*fake_face_cb)(face_recognition_ret_t *face);

    void (*pass_face_cb)(face_recognition_ret_t *face, uint8_t ir_check);

    void (*lcd_refresh_cb)(void);

    void (*lcd_convert_cb)(void);

    void (*lcd_close_cb)(void);
} face_lib_callback_t;

///////////////////////////////////////////////////////////////////////////////
extern volatile uint8_t face_lib_draw_flag;
extern volatile uint8_t rgb_buf_index;
///////////////////////////////////////////////////////////////////////////////
//get face_lib version
char *face_lib_version(void);

//init module
uint8_t face_lib_init_module(void);

uint8_t face_lib_regisiter_callback(face_lib_callback_t *cfg_cb);

//cal camera pic face feature
void face_lib_run(face_recognition_cfg_t *cfg);

//compare two face
float face_lib_compare_score(int8_t *feature0, int8_t *feature1);
///////////////////////////////////////////////////////////////////////////////
/* uart protocol */
/* clang-format off */
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

typedef struct
{
    uint8_t threshold;

    uint8_t auto_add;
    uint8_t spec_uid;
    uint8_t uid[16];

    uint8_t *jpeg_buf;
    uint32_t jpeg_size;
    uint8_t jpeg_sha256[32];

} cal_pic_fea_t;

extern cal_pic_fea_t cal_pic_cfg;

typedef int8_t (*cal_pic_send_ret)(uint8_t code, char *msg, int8_t feature[FEATURE_DIMENSION], uint8_t *uid, float prob);

typedef struct
{
    char *cmd;
    void (*cmd_cb)(cJSON *root);
} protocol_custom_cb_t;

#include "image_op.h"
#include "uart_recv.h"

uint8_t protocol_send_init_done(void);

void protocol_prase(unsigned char *protocol_buf);
void protocol_cal_pic_fea(cal_pic_fea_t *cal_cfg, cal_pic_send_ret send_ret);
uint8_t protocol_send_cal_pic_result(uint8_t code, char *msg, int8_t feature[FEATURE_DIMENSION], uint8_t *uid, float prob);

uint8_t protocol_send_face_info(face_obj_t *obj,
                                float score, uint8_t uid[UID_LEN], int8_t feature[FEATURE_DIMENSION],
                                uint32_t total, uint32_t current, uint64_t *time);

cJSON *protocol_gen_header(char *cmd, uint8_t code, char *msg);
uint8_t protocol_send(const cJSON *const send);
uint8_t protocol_regesiter_user_cb(protocol_custom_cb_t *cb_list, uint32_t ncb);

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
int gc0328_dual_init(camera_t *camera);
int gc0328_single_init(camera_t *camera);
void open_gc0328_650();
extern volatile uint8_t g_dvp_finish_flag;
///////////////////////////////////////////////////////////////////////////////
void sipeed_spi_send_data_dma(uint8_t spi_num, uint8_t chip_select, uint8_t dma_chn, const uint8_t *data_buf, size_t buf_len);
///////////////////////////////////////////////////////////////////////////////
#endif
