#ifndef __SD_OP_H
#define __SD_OP_H

#include <stdint.h>

#include "face_lib.h"

void sd_init_fatfs(void);

uint8_t sd_save_img_ppm(char *fname, image_t *img);

#endif
