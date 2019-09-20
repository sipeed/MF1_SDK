#include "kpu.h"
#include "layer.h"

typedef struct 
{
    uint16_t w;
    uint16_t h;
    uint8_t ch;
    uint8_t *buf;
} conv_img_t;

void blur_detect_init(uint16_t w, uint16_t h, uint8_t ch);
float blur_detect_run(uint8_t *buf_in);
void blur_detect_deinit();

