#ifndef __CAMERA_H
#define __CAMERA_H

#include "global_config.h"
#include "stdint.h"

extern uint8_t kpu_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3];
extern uint8_t display_image[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2];


#endif

