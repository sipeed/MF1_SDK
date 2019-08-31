#include "lcd_dis.h"

#include <stdio.h>
#include <string.h>

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
        printf("id too big\r\n");
        return 1;
    }

    if (((list_id_table[id / 32] >> (id % 32)) & 0x1) == 0x1)
    {
        // printf("id already exist\r\n");
        return 1;
    }

    list_id_table[id / 32] |= (0x1 << (id % 32));

    // printf("id not exist\r\n");

    return 0;
}

static void lcd_dis_list_draw_str(uint8_t *image, uint16_t img_w, uint16_t img_h, dis_str_t *dis_str)
{
    image_rgb565_draw_string(image, dis_str->str,
                             dis_str->x, dis_str->y,
                             dis_str->color, (dis_str->bg_color == 1) ? NULL : &(dis_str->bg_color), img_w);
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
    img_dst.img_addr = (uint16_t *)kpu_image[1];
#else
    img_dst.img_addr = (uint16_t *)kpu_image_tmp;
#endif
    img_dst.x = dis_pic->x;
    img_dst.y = dis_pic->y;
    img_dst.w = dis_pic->w;
    img_dst.h = dis_pic->h;

    my_w25qxx_read_data((uint32_t)(dis_pic->flash_addr), img_dst.img_addr, img_size, W25QXX_STANDARD);

    image_rgb565_mix_pic_with_alpha(&img_src, &img_dst, dis_pic->alpha);
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
        printf("unk dis type\r\n");
        break;
    }
    return;
}

lcd_dis_t *lcd_dis_list_add_str(int id, int auto_del,
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

            lcd_dis->dis = (void *)dis_str;

            if (list_rpush(lcd_dis_list, list_node_new(lcd_dis)) == NULL)
            {
                printf("list_rpush failed\r\n");
                free(lcd_dis);
                return NULL;
            }
            return lcd_dis;
        }
        else
        {
            printf("failed\r\n");
            free(lcd_dis);
            return NULL;
        }
    }
    else
    {
        printf("failed\r\n");
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
                printf("list_rpush failed\r\n");
                free(lcd_dis);
                return NULL;
            }
            return lcd_dis;
        }
        else
        {
            printf("failed\r\n");
            free(lcd_dis);
            return NULL;
        }
    }
    else
    {
        printf("failed\r\n");
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
        printf("init list failed\r\n");
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
