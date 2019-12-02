#include "lcd_dis.h"

#include <stdio.h>
#include <string.h>

#include "printf.h"

#include "image_op.h"
#include "board.h"
#include "face_lib.h"
#include "system_config.h"
///////////////////////////////////////////////////////////////////////////////
#define LIST_MAX_SIZE 256
///////////////////////////////////////////////////////////////////////////////
list_t *lcd_dis_list = NULL;
///////////////////////////////////////////////////////////////////////////////
static uint32_t list_id_table[LIST_MAX_SIZE / 32];
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static uint8_t lcd_dis_list_check_id(int id)
{
    if (id > LIST_MAX_SIZE)
    {
        printk("id too big\r\n");
        return 1;
    }

    if (((list_id_table[id / 32] >> (id % 32)) & 0x1) == 0x1)
    {
        // printk("id already exist\r\n");
        return 1;
    }

    list_id_table[id / 32] |= (0x1 << (id % 32));

    // printk("id not exist\r\n");

    return 0;
}

//字体：新宋体
//取模方式： 阴码，逐列式，顺向
#if 1 //GB2312 字库
uint8_t lcd_dis_get_zhCN_dat(uint8_t *zhCN_char, uint8_t *zhCN_dat, uint8_t size)
{
    uint8_t ch, cl;
    uint32_t font_offset;

    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);

    ch = *zhCN_char;
    cl = *(++zhCN_char);

    if (ch < 0xa1 || cl < 0xa0 ||
        ch > 0xf7 ||
        (ch >= 0xaa && ch <= 0xaf))
    {
        return 0;
    }

    ch -= 0xa1;
    cl -= 0xa0;

    font_offset = (ch * 96 + cl) * csize;
    switch (size)
    {
    case 16:
        my_w25qxx_read_data((uint32_t)(FONT_16x16_ADDR + font_offset), zhCN_dat, csize, W25QXX_STANDARD);
        break;
    case 24:
        my_w25qxx_read_data((uint32_t)(FONT_24x24_ADDR + font_offset), zhCN_dat, csize, W25QXX_STANDARD);
        break;
    case 32:
        my_w25qxx_read_data((uint32_t)(FONT_32x32_ADDR + font_offset), zhCN_dat, csize, W25QXX_STANDARD);
        break;
    default:
        return 0;
        break;
    }

    return 1;
}
#else //GBK 字库
uint8_t lcd_dis_get_zhCN_dat(uint8_t *zhCN_char, uint8_t *zhCN_dat, uint8_t size)
{
    uint8_t ch, cl;
    uint32_t font_offset;

    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);

    ch = *zhCN_char;
    cl = *(++zhCN_char);

    if (ch < 0x81 || cl < 0x40 || ch == 0xff || cl == 0xff)
    {
        return 0;
    }

    if (cl < 0x7f)
        cl -= 0x40;
    else
        cl -= 0x41;
    ch -= 0x81;

    font_offset = (190 * ch + cl) * csize;

    my_w25qxx_read_data((uint32_t)(FONT_GBK16_ADDR + font_offset), zhCN_dat, csize, W25QXX_STANDARD);

    return 1;
}
#endif

static void lcd_dis_list_draw_str(uint8_t *image, uint16_t img_w, uint16_t img_h, dis_str_t *dis_str)
{
    if (dis_str->zh_CN)
    {
        image_rgb565_draw_zhCN_string(image, dis_str->str, dis_str->size, dis_str->x, dis_str->y,
                                      dis_str->color, (dis_str->bg_color == 1) ? NULL : &(dis_str->bg_color),
                                      img_w, img_h,
                                      lcd_dis_get_zhCN_dat);
    }
    else
    {
        image_rgb565_draw_string(image, dis_str->str, dis_str->size, dis_str->x, dis_str->y,
                                 dis_str->color, (dis_str->bg_color == 1) ? NULL : &(dis_str->bg_color), img_w, img_h);
    }
    return;
}

static void lcd_dis_list_draw_pic(uint8_t *image, uint16_t img_w, uint16_t img_h, dis_pic_t *dis_pic)
{
    mix_image_t img_src, img_dst;

    img_src.img_addr = (uint16_t *)image;
    img_src.w = img_w;
    img_src.h = img_h;
    img_src.x = 0;
    img_src.y = 0;

    uint32_t img_size = dis_pic->w * dis_pic->h * 2;
#if (CONFIG_CAMERA_GC0328_DUAL == 0)
    img_dst.img_addr = (uint16_t *)_IOMEM_ADDR(kpu_image[1]);
#else
    img_dst.img_addr = (uint16_t *)_IOMEM_ADDR(kpu_image_tmp);
#endif
    img_dst.x = dis_pic->x;
    img_dst.y = dis_pic->y;
    img_dst.w = dis_pic->w;
    img_dst.h = dis_pic->h;

#if CONFIG_LCD_TYPE_SIPEED
    w25qxx_read_data((uint32_t)(dis_pic->flash_addr), img_dst.img_addr, img_size);
#else //TODO: fix here on small lcd
    my_w25qxx_read_data((uint32_t)(dis_pic->flash_addr), img_dst.img_addr, img_size, W25QXX_STANDARD); //4ms
#endif
    //FIXME: 这里可以更新到最新小内存占用的。
    image_rgb565_mix_pic_with_alpha(&img_src, &img_dst, dis_pic->alpha); //12ms
    return;
}

void lcd_dis_list_display(uint8_t *image, uint16_t img_w, uint16_t img_h, lcd_dis_t *lcd_dis)
{
    switch (lcd_dis->type)
    {
    case DIS_TYPE_STR:
        lcd_dis_list_draw_str(image, img_w, img_h, (dis_str_t *)(lcd_dis->dis));
        break;
    case DIS_TYPE_PIC:
        lcd_dis_list_draw_pic(image, img_w, img_h, (dis_str_t *)(lcd_dis->dis));
        break;
    default:
        printk("unk dis type\r\n");
        break;
    }
    return;
}

lcd_dis_t *lcd_dis_list_add_str(int id, int auto_del, uint16_t size, uint16_t zh_CN,
                                char *str, uint16_t x, uint16_t y,
                                uint16_t color, uint16_t bg_color)
{
    lcd_dis_t *lcd_dis = (lcd_dis_t *)malloc(sizeof(lcd_dis_t));

    if (lcd_dis)
    {
        lcd_dis->type = DIS_TYPE_STR;
        lcd_dis->auto_del = auto_del;

        if (lcd_dis_list_check_id(id))
        {
            lcd_dis_list_del_by_id(id);
            //recheck
            lcd_dis_list_check_id(id);
        }
        lcd_dis->id = id;

        dis_str_t *dis_str = (dis_str_t *)malloc(sizeof(dis_str_t));
        if (dis_str)
        {
            dis_str->x = x;
            dis_str->y = y;
            dis_str->color = color;
            dis_str->bg_color = bg_color;
            dis_str->str = str;
            dis_str->size = size;
            dis_str->zh_CN = zh_CN;

            lcd_dis->dis = (void *)dis_str;

            if (list_rpush(lcd_dis_list, list_node_new(lcd_dis)) == NULL)
            {
                printk("list_rpush failed\r\n");
                free(lcd_dis);
                return NULL;
            }
            return lcd_dis;
        }
        else
        {
            printk("failed\r\n");
            free(lcd_dis);
            return NULL;
        }
    }
    else
    {
        printk("failed\r\n");
        return NULL;
    }
    return NULL;
}

lcd_dis_t *lcd_dis_list_add_pic(int id, int auto_del,
                                uint32_t addr, uint32_t alpha,
                                uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h)
{
    lcd_dis_t *lcd_dis = (lcd_dis_t *)malloc(sizeof(lcd_dis_t));

    if (lcd_dis)
    {
        lcd_dis->type = DIS_TYPE_PIC;
        lcd_dis->auto_del = auto_del;
        if (lcd_dis_list_check_id(id))
        {
            lcd_dis_list_del_by_id(id);
        }
        lcd_dis->id = id;

        dis_pic_t *dis_pic = (dis_pic_t *)malloc(sizeof(dis_pic_t));
        if (dis_pic)
        {
            dis_pic->x = x;
            dis_pic->y = y;
            dis_pic->w = w;
            dis_pic->h = h;

            dis_pic->flash_addr = addr;
            dis_pic->alpha = alpha;

            lcd_dis->dis = (void *)dis_pic;

            if (list_rpush(lcd_dis_list, list_node_new(lcd_dis)) == NULL)
            {
                printk("list_rpush failed\r\n");
                free(lcd_dis);
                return NULL;
            }
            return lcd_dis;
        }
        else
        {
            printk("failed\r\n");
            free(lcd_dis);
            return NULL;
        }
    }
    else
    {
        printk("failed\r\n");
        return NULL;
    }

    return NULL;
}

uint8_t lcd_dis_list_init(void)
{
    memset(list_id_table, 0, sizeof(list_id_table));
    lcd_dis_list = list_new();
    if (lcd_dis_list == NULL)
    {
        printk("init list failed\r\n");
        return 1;
    }
    return 0;
}

int lcd_dis_list_del_by_id(int id)
{
    list_node_t *node = NULL;
    list_iterator_t *lcd_dis_list_iterator = list_iterator_new(lcd_dis_list, LIST_HEAD);

    if (lcd_dis_list_iterator)
    {
        while ((node = list_iterator_next(lcd_dis_list_iterator)))
        {
            lcd_dis_t *lcd_dis = (lcd_dis_t *)node->val;
            if (lcd_dis->id == id)
            {
                lcd_dis_list_free(lcd_dis);
                list_remove(lcd_dis_list, node);
                list_id_table[id / 32] &= ~(1 << (id % 32));
                return id;
            }
        }
    }
    return -1;
}

void lcd_dis_list_del_all(void)
{
    list_node_t *node = NULL;
    list_iterator_t *lcd_dis_list_iterator = list_iterator_new(lcd_dis_list, LIST_HEAD);

    if (lcd_dis_list_iterator)
    {
        while ((node = list_iterator_next(lcd_dis_list_iterator)))
        {
            lcd_dis_t *lcd_dis = (lcd_dis_t *)node->val;
            lcd_dis_list_free(lcd_dis);
            list_remove(lcd_dis_list, node);
        }
    }
    memset(list_id_table, 0, sizeof(list_id_table));
    return;
}

void lcd_dis_list_free(lcd_dis_t *lcd_dis)
{
    if (lcd_dis == NULL)
        return;

    if (lcd_dis->type == DIS_TYPE_STR)
    {
        dis_str_t *dis_str = (dis_str_t *)lcd_dis->dis;
        if (dis_str->str)
        {
            free(dis_str->str);
        }
        free(dis_str);
    }
    else if (lcd_dis->type == DIS_TYPE_PIC)
    {
        free(lcd_dis->dis);
    }
    free(lcd_dis);
    return;
}
