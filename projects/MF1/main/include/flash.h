#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>
#include "system_config.h"

//每一次大的更改配置结构的时候建议修改版本
#define CFG_VERSION (0.1f)
#define CFG_HEADER (0x55AA5558)

typedef struct _board_cfg
{
    uint8_t cfg_sha256[32];

    uint32_t header;
    float version;
    uint32_t cfg_right_flag;

    struct
    {
        struct
        {
            uint64_t lcd_dir : 8;
            uint64_t lcd_flip : 1;
            uint64_t lcd_hmirror : 1;
            uint64_t resv1 : 6;
            uint64_t cam_flip : 1;
            uint64_t cam_hmirror : 1;
            uint64_t resv : 46;
        } lcd_cam;

        struct
        {
            uint64_t port_tx : 8;
            uint64_t port_rx : 8;
            uint64_t log_tx : 8;
            uint64_t log_rx : 8;
            uint64_t relay_low : 8;
            uint64_t relay_high : 8;
            uint64_t key : 8;
            uint64_t key_dir : 1;
            uint64_t resv : 5;
        } uart_relay_key;
    } brd_hard_cfg;

    struct
    {
        float out_threshold;

        struct
        {
            uint64_t out_fea : 2;
            uint64_t auto_out_fea : 1;
            uint64_t pkt_fix : 1;
            uint64_t relay_open_s : 8;
            uint64_t out_interval_ms : 16;
            uint64_t port_baud : 32;
            uint64_t resv : 4;
        } cfg;
    } brd_soft_cfg;

    void *user_custom_cfg;

    uint8_t wifi_ssid[32];
    uint8_t wifi_passwd[32];

} board_cfg_t __attribute__((aligned(8)));

typedef struct
{
    uint32_t stat; //0,rgb; 1,ir; 2,ir+rgb
    int8_t fea_rgb[196];
    int8_t fea_ir[196];
} face_fea_t;

typedef struct _face_info_t
{
    uint32_t index;
    uint8_t uid[UID_LEN];
    uint32_t valid;
    // float info[196];
    face_fea_t info;
    char name[NAME_LEN];
    char note[NOTE_LEN];
} face_info_t;

typedef struct _face_save_info_t
{
    uint32_t header;
    uint32_t version;
    uint32_t number;
    uint32_t checksum;
    uint32_t face_info_index[FACE_DATA_MAX_COUNT / 32 + 1];
} face_save_info_t __attribute__((aligned(8)));

extern volatile face_save_info_t g_face_save_info;
extern volatile board_cfg_t g_board_cfg;
extern volatile int flash_all_face_have_ir_fea;

void flash_init(void);
int flash_delete_face_info(uint32_t id);
int flash_save_face_info(face_fea_t *features, uint8_t *uid, uint32_t valid, char *name, char *note, uint8_t *ret_uid);
int flash_delete_face_all(void);
uint32_t flash_get_wdt_reboot_count(void);
int flash_get_saved_feature(face_fea_t *feature, uint32_t index);
int flash_get_saved_feature_number(void);
int flash_get_saved_faceinfo(face_info_t *info, uint32_t index);

uint32_t flash_get_id_by_uid(uint8_t *uid);

int flash_get_saved_feature_sha256(uint8_t sha256[32]);
int flash_get_saved_uid_feature(uint32_t index, uint8_t uid[16], face_fea_t *feature);

int flash_update_face_info(face_info_t *face_info);

int flash_get_face_info(face_info_t *face_info, uint32_t id);

uint8_t flash_load_cfg(board_cfg_t *cfg);
uint8_t flash_save_cfg(board_cfg_t *cfg);
uint8_t flash_cfg_print(board_cfg_t *cfg);
uint8_t flash_cfg_set_default(board_cfg_t *cfg);

#endif
