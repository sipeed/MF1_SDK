

#include "camera.h"

uint8_t kpu_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(128)));
uint8_t display_image[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2] __attribute__((aligned(64)));

