#ifndef __CAMERA_H
#define __CAMERA_H

#include "stdint.h"

typedef enum _camera_type
{
    CAM_OV2640 = 0,
    CAM_GC0328_SINGLE,
    CAM_GC0328_DUAL,
} camera_type_t;

typedef struct _camera camera_t;
typedef struct _camera
{
    uint8_t slv_id;

    int (*camera_config)(camera_t *camera);
    int (*camera_set_hmirror)(camera_t *camera, uint8_t val);
    int (*camera_set_vflip)(camera_t *camera, uint8_t val);
    int (*camera_read_id)(camera_t *camera, uint16_t *manuf_id, uint16_t *device_id);
} camera_t;

int camera_init(camera_type_t type);

int camera_read_id(uint16_t *manuf_id, uint16_t *device_id);
int camera_set_hmirror(uint8_t val);
int camera_set_vflip(uint8_t val);
int camera_config(void);

#endif
