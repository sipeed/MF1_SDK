#include "face_cb.h"

#include "system_config.h"

#include "board.h"
#include "flash.h"

#if CONFIG_LCD_TYPE_ST7789
#include "lcd_st7789.h"
#elif CONFIG_LCD_TYPE_SSD1963
#include "lcd_ssd1963.h"
#endif

#if CONFIG_LCD_TYPE_ST7789
#if LCD_240240
extern uint8_t lcd_image[LCD_W * LCD_H * 2];
#else
extern const uint8_t *lcd_image;
#endif
#endif

#if CONFIG_LCD_TYPE_ST7789
static void display_fit_lcd(uint8_t *display_image, uint8_t *lcd_image)
{
#if LCD_240240
    int x0, x1, x, y;
    uint16_t r, g, b;
    uint16_t *src = (uint16_t *)display_image;
    uint16_t *dest = (uint16_t *)lcd_image;
    x0 = (IMG_W - LCD_W) / 2;
    x1 = x0 + LCD_W;
    for(y = 0; y < LCD_H; y++)
    {
        memcpy(lcd_image + y * LCD_W * 2, display_image + (y * IMG_W + x0) * 2, LCD_W * 2);
    }
#endif
    return;
}

static uint16_t Fast_AlphaBlender(uint32_t x, uint32_t y, uint32_t Alpha)
{
    x = (x | (x << 16)) & 0x7E0F81F;
    y = (y | (y << 16)) & 0x7E0F81F;
    uint32_t result = ((((x - y) * Alpha) >> 5) + y) & 0x7E0F81F;
    return (uint16_t)((result & 0xFFFF) | (result >> 16));
}

//aplha is stack up picture's diaphaneity
//aplha指的是叠加的图片的透明度
static void display_fit_lcd_with_alpha(uint8_t *pic_addr, uint8_t alpha)
{
#if LCD_240240
    int x0, x1, x, y;
    my_w25qxx_read_data(pic_addr, lcd_image, IMG_LCD_SIZE, W25QXX_STANDARD);
    uint16_t *s0 = (uint16_t *)display_image;
    uint16_t *s1 = (uint16_t *)lcd_image;
    x0 = (IMG_W - LCD_W) / 2;
    x1 = x0 + LCD_W;

    for(y = 0; y < LCD_H; y++)
    {
        for(x = 0; x < LCD_W; x++)
        {
            s1[y * LCD_W + x] = Fast_AlphaBlender((uint32_t)s0[y * IMG_W + x + x0], (uint32_t)s1[y * LCD_W + x], (uint32_t)alpha / 8);
        }
    }
#endif
    return;
}
#endif

#if CONFIG_LCD_TYPE_ST7789
// static void draw_edge(uint8_t *gram, face_obj_t *obj_info, uint32_t color, uint8_t roate, uint8_t scale, char *str)
void draw_edge(uint32_t *gram, face_obj_t *obj, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4, x1, y1, x2, y2;

    x1 = obj->x1;
    y1 = obj->y1;
    x2 = obj->x2;
    y2 = obj->y2;

    if(x1 <= 0)
        x1 = 1;
    if(x2 >= IMG_W - 1)
        x2 = IMG_W - 2;
    if(y1 <= 0)
        y1 = 1;
    if(y2 >= IMG_H - 1)
        y2 = IMG_H - 2;

    addr1 = gram + (IMG_W * y1 + x1) / 2;
    addr2 = gram + (IMG_W * y1 + x2 - 8) / 2;
    addr3 = gram + (IMG_W * (y2 - 1) + x1) / 2;
    addr4 = gram + (IMG_W * (y2 - 1) + x2 - 8) / 2;
    for(uint32_t i = 0; i < 4; i++)
    {
        *addr1 = data;
        *(addr1 + IMG_W / 2) = data;
        *addr2 = data;
        *(addr2 + IMG_W / 2) = data;
        *addr3 = data;
        *(addr3 + IMG_W / 2) = data;
        *addr4 = data;
        *(addr4 + IMG_W / 2) = data;
        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }
    addr1 = gram + (IMG_W * y1 + x1) / 2;
    addr2 = gram + (IMG_W * y1 + x2 - 2) / 2;
    addr3 = gram + (IMG_W * (y2 - 8) + x1) / 2;
    addr4 = gram + (IMG_W * (y2 - 8) + x2 - 2) / 2;
    for(uint32_t i = 0; i < 8; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;
        addr1 += IMG_W / 2;
        addr2 += IMG_W / 2;
        addr3 += IMG_W / 2;
        addr4 += IMG_W / 2;
    }
}
#elif CONFIG_LCD_TYPE_SSD1963
static void RotationRight90_rectangle(rectangle_t rec, rectangle_t *roate)
{
    roate->x = (uint16_t)(LCD_H - rec.y - rec.h);
    roate->y = (uint16_t)(rec.x - 1);

    roate->w = (uint16_t)(rec.h);
    roate->h = (uint16_t)(rec.w);
}

static void draw_edge(uint8_t *gram, face_obj_t *obj_info, uint32_t color, uint8_t roate, uint8_t scale, char *str)
{
    rectangle_t rec, rec_roate;

    uint16_t x1, y1, x2, y2;

    x1 = (uint16_t)(obj_info->x1);
    y1 = (uint16_t)(obj_info->y1);
    x2 = (uint16_t)(obj_info->x2);
    y2 = (uint16_t)(obj_info->y2);

    if(x1 <= 0)
        x1 = 1;
    if(x2 >= 319)
        x2 = 318;
    if(y1 <= 0)
        y1 = 1;
    if(y2 >= 239)
        y2 = 238;

    rec.x = x1;
    rec.y = y1;
    rec.w = x2 - x1;
    rec.h = y2 - y1;

    if(roate)
    {
        RotationRight90_rectangle(rec, &rec_roate);
    } else
    {
        rec_roate.x = rec.x;
        rec_roate.y = rec.y;
        rec_roate.w = rec.w;
        rec_roate.h = rec.h;
    }

    rec_roate.x *= scale;
    rec_roate.y *= scale;
    rec_roate.w *= scale;
    rec_roate.h *= scale;

    if(roate)
    {
        lcd_draw_rectangle(rec_roate, 2, color);
        if(str)
            lcd_draw_string(rec_roate.x, rec_roate.y, str, color);
    } else
    {
        lcd_draw_rectangle(rec_roate, 2, color);
        if(str)
            lcd_draw_string(rec_roate.x, rec_roate.y, str, color);
    }
}
#endif

void record_face(face_recognition_ret_t *ret)
{
    uint32_t face_cnt = ret->result->face_obj_info.obj_number;
    get_date_time();
    for(uint32_t i = 0; i < face_cnt; i++)
    {
        if(flash_save_face_info(NULL, ret->result->face_obj_info.obj[i].feature,
                                NULL, 1, NULL, NULL, NULL) < 0) //image, feature, uid, valid, name, note
        {
            printf("Feature Full\n");
            break;
        }
#if CONFIG_LCD_TYPE_ST7789
        display_fit_lcd_with_alpha(IMG_LOG_OK_ADDRESS, 128);
        lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image); //7.5ms
#elif CONFIG_LCD_TYPE_SSD1963
        lcd_draw_string(LCD_OFT, IMG_H - 16, "Recording...", lcd_color(0xff, 0, 0));
#endif
        set_RGB_LED(GLED);
        msleep(500);
        msleep(500);
        set_RGB_LED(0);
    }
    return;
}

void box_false_face(face_recognition_ret_t *ret)
{
    char display_string[32];
    uint32_t face_cnt = ret->result->face_obj_info.obj_number;
    for(uint32_t i = 0; i < face_cnt; i++)
    {
        face_obj_t *obj = &ret->result->face_obj_info.obj[i];
        if(obj->pass == false) //draw false facess
        {
            sprintf(display_string, "%.2f", obj->score);
#if CONFIG_LCD_TYPE_ST7789
            draw_edge((uint32_t *)display_image, obj, RED);
#elif CONFIG_LCD_TYPE_SSD1963
            draw_edge(display_image_rgb888, obj, lcd_color(0xff, 0x00, 0x00), 0, 1, NULL);
#endif
        }
    }
    return;
}

/**************************************************************************/
//
/**************************************************************************/

void recognition_draw_callback(uint8_t *cam_image)
{
#if CONFIG_LCD_TYPE_ST7789
    display_fit_lcd(cam_image, lcd_image);
    lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image);
#elif CONFIG_LCD_TYPE_SSD1963
    lcd_draw_picture(0, 0, LCD_W, LCD_H, display_image_rgb888);
#endif
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void utils_display_pass(void)
{
    display_fit_lcd_with_alpha(IMG_REC_OK_ADDRESS, 128);
    lcd_draw_picture(0, 0, LCD_W, LCD_H, (uint32_t *)lcd_image); //7.5ms
    msleep(300); //display
    return;
}
