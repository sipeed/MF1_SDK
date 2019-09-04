#ifndef __LCD_DIS_H
#define __LCD_DIS_H

#include <stdint.h>
#include "list.h"

/* clang-format off */
enum{
    DIS_TYPE_STR=0,
    DIS_TYPE_PIC,
};

typedef struct 
{
    uint16_t x;
    uint16_t y;

    uint16_t color;
    uint16_t bg_color;

    uint16_t zh_CN;
    uint16_t size;

    char *str;//must malloc
}dis_str_t;

typedef struct 
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint32_t flash_addr;
    uint32_t alpha;
}dis_pic_t;

typedef struct 
{
    int id;
    int type;//0:str,1:pic
    int auto_del;
    void * dis;
}lcd_dis_t;
/* clang-format on */
///////////////////////////////////////////////////////////////////////////////

extern list_t *lcd_dis_list;
///////////////////////////////////////////////////////////////////////////////
uint8_t lcd_dis_list_init(void);
void lcd_dis_list_del_all(void);
int lcd_dis_list_del_by_id(int id);
void lcd_dis_list_free(lcd_dis_t *lcd_dis);

void lcd_dis_list_display(uint8_t *image, uint16_t img_w, uint16_t img_h, lcd_dis_t *lcd_dis);

lcd_dis_t *lcd_dis_list_add_str(int id, int auto_del, uint16_t size, uint16_t zh_CN,
                                char *str, uint16_t x, uint16_t y,
                                uint16_t color, uint16_t bg_color);

lcd_dis_t *lcd_dis_list_add_pic(int id, int auto_del,
                                uint32_t addr, uint32_t alpha,
                                uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h);

uint8_t lcd_dis_get_zhCN_dat(uint8_t *zhCN_char, uint8_t *zhCN_dat, uint8_t size);

///////////////////////////////////////////////////////////////////////////////

#endif
