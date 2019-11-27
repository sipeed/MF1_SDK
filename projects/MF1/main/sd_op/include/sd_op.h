#ifndef __SD_OP_H
#define __SD_OP_H

#include <stdint.h>

#include "ff.h"

#include "face_lib.h"

#include "face_cb.h"

extern FATFS fs;

uint8_t sd_init_fatfs(void);

uint8_t sd_save_img_ppm(char *fname, image_t *img, uint8_t r8g8b8);

uint8_t sd_chk_ota_file_available(char *path);

#endif
