#include "sd_op.h"

#include <stdio.h>

#include "sysctl.h"
#include "sha256.h"

#include "lcd_dis.h"
#include "sd_card.h"

FATFS fs;

extern volatile uint8_t dis_flag;

///////////////////////////////////////////////////////////////////////////////
static const uint8_t new_version[] = {/* 检 */ 0xBC, 0xEC,
                                      /* 测 */ 0xB2, 0xE2,
                                      /* 到 */ 0xB5, 0xBD,
                                      /* 新 */ 0xD0, 0xC2,
                                      /* 版 */ 0xB0, 0xE6,
                                      /* 本 */ 0xB1, 0xBE,
                                      /* 固 */ 0xB9, 0xCC,
                                      /* 件 */ 0xBC, 0xFE,
                                      0x0};

static const uint8_t fm_err[] = {/* 固 */ 0xB9, 0xCC,
                                 /* 件 */ 0xBC, 0xFE,
                                 /* 校 */ 0xD0, 0xA3,
                                 /* 验 */ 0xD1, 0xE9,
                                 /* 失 */ 0xCA, 0xA7,
                                 /* 败 */ 0xB0, 0xDC,
                                 0x0};

static const uint8_t ensure_fm[] = {/* 请 */ 0xC7, 0xEB,
                                    /* 确 */ 0xC8, 0xB7,
                                    /* 认 */ 0xC8, 0xCF,
                                    /* 固 */ 0xB9, 0xCC,
                                    /* 件 */ 0xBC, 0xFE,
                                    /* 正 */ 0xD5, 0xFD,
                                    /* 确 */ 0xC8, 0xB7,
                                    /* ！ */ 0xA3, 0xA1,
                                    0x0};

static const uint8_t update_fm[] = {/* 正 */ 0xD5, 0xFD,
                                    /* 在 */ 0xD4, 0xDA,
                                    /* 更 */ 0xB8, 0xFC,
                                    /* 新 */ 0xD0, 0xC2,
                                    0x0};

static const uint8_t about5min[] = {/* 大 */ 0xB4, 0xF3,
                                    /* 概 */ 0xB8, 0xC5,
                                    /* 需 */ 0xD0, 0xE8,
                                    /* 要 */ 0xD2, 0xAA,
                                    /* 5 */ 0x35,
                                    /* 秒 */ 0xC3, 0xEB,
                                    /* 钟 */ 0xD6, 0xD3,
                                    0x0};

static const uint8_t dontclose[] = {/* 请 */ 0xC7, 0xEB,
                                    /* 不 */ 0xB2, 0xBB,
                                    /* 要 */ 0xD2, 0xAA,
                                    /* 关 */ 0xB9, 0xD8,
                                    /* 闭 */ 0xB1, 0xD5,
                                    /* 电 */ 0xB5, 0xE7,
                                    /* 源 */ 0xD4, 0xB4,
                                    0x00};

///////////////////////////////////////////////////////////////////////////////
uint8_t sd_init_fatfs(void)
{
    uint64_t tim = sysctl_get_time_us();
    uint8_t ret = SD_Initialize();
    printk("init sd us :%ld\r\n", sysctl_get_time_us() - tim);

    if (ret != 0)
    {
        printk("ret: 0x%02X\r\n", ret);
        return 1;
    }

    uint32_t sd_size = SD_GetSectorCount();
    printk("SD Type: %d Size: %dMB\r\n", SD_Type, sd_size >> 11);

    FRESULT res = f_mount(&fs);
    printk(res == FR_OK ? "mount ok!\r\n"
                        : "mount error!%d\r\n",
           res);
    return 0;
}

/* 升级文件放在OTA文件夹下 */
/* 文件名为: OTA.BIN */
/* 升级文件格式要求，普通的bin文件进行sha256fix，然后头部再加个版本号(float) */
uint8_t sd_chk_ota_file_available(char *path)
{
    uint8_t ret = 0;
    char p[32];

    FRESULT res;
    FIL fp;

    sprintf(p, "%s/OTA.BIN", path);

    res = f_open(&fs, &fp, p, FA_READ);
    if (res != FR_OK)
    {
        printk("no update firmware\r\n");
        return 1;
    }

    /* 读出文件 */
    size_t fsize = f_size(&fp);
    printk("file size:%ld\r\n", fsize);

    uint8_t *file = malloc(fsize + 4);
    if (file == NULL)
    {
        printk("malloc error\r\n");
        f_close(&fp);
        return 1;
    }

    uint32_t read = 0;
    res = f_read(&fp, file, fsize, &read);
    if (res != FR_OK || read != fsize)
    {
        printk("read file failed\r\n");
        f_close(&fp);
        ret = 1;
        goto _exit;
    }

    f_close(&fp);

    ///////////////////////////////////////
    // for (uint32_t i = 0; i < 9; i++)
    // {
    //     printk("%d: %02X\r\n", i, *(file + i));
    // }

    // for (uint32_t i = (fsize - 32); i < fsize; i++)
    // {
    //     printk("%d: %02X\r\n", i, *(file + i));
    // }
    ///////////////////////////////////////

    /* 这里和当前版本的进行比对 */
    uint8_t tmp, sha256[32];
    uint32_t firmware_size = 0;

    float ota_version = (float)*((float *)file);

    if (ota_version > (float)FIRMWARE_VERSION)
    {
        /* 打开屏幕背光，显示提示 */
        set_lcd_bl(1);

        while (dis_flag)
        {
        }; //等待中断刷完屏

        image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), new_version, 24,
                                      (SIPEED_LCD_W - ((sizeof(new_version)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2,
                                      WHITE, NULL,
                                      SIPEED_LCD_W, SIPEED_LCD_H,
                                      lcd_dis_get_zhCN_dat);

        /* sd卡中的固件比较新，就进行升级，要进行校验 */
        sprintf(p, "version: %.2f > %.2f", ota_version, (float)FIRMWARE_VERSION);
        printk("got new %s\r\n", p);

        printk("new firmware: %02X %s\r\n", *(file + 4), *(file + 4) ? "encrypted" : "normal");

        firmware_size = (uint32_t)((*(file + 8) << 24) | (*(file + 7) << 16) | (*(file + 6) << 8) | (*(file + 5)));

        printk("new firmware len:%d\r\n", firmware_size);

        if (firmware_size != (fsize - 9 - 32))
        {
            printk("firmware_size error\r\n");
            while (dis_flag)
            {
            }; //等待中断刷完屏
            image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), fm_err, 24,
                                          (SIPEED_LCD_W - ((sizeof(fm_err)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 24,
                                          RED, NULL,
                                          SIPEED_LCD_W, SIPEED_LCD_H,
                                          lcd_dis_get_zhCN_dat);

            while (dis_flag)
            {
            }; //等待中断刷完屏
            image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), ensure_fm, 24,
                                          (SIPEED_LCD_W - ((sizeof(ensure_fm)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 48,
                                          RED, NULL,
                                          SIPEED_LCD_W, SIPEED_LCD_H,
                                          lcd_dis_get_zhCN_dat);
            msleep(500);
            ret = 1;
            goto _exit;
        }

        sha256_hard_calculate((uint8_t *)(file + 4), firmware_size + 5, sha256);

        // for (uint8_t i = 0; i < 32; i++)
        // {
        //     printk("%d: %02X\r\n", i, sha256[i]);
        // }

        if (memcmp(sha256, file + (fsize - 32), 32) != 0)
        {
            printk("sha256 check failed\r\n");

            while (dis_flag)
            {
            }; //等待中断刷完屏
            image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), fm_err, 24,
                                          (SIPEED_LCD_W - ((sizeof(fm_err)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 24,
                                          RED, NULL,
                                          SIPEED_LCD_W, SIPEED_LCD_H,
                                          lcd_dis_get_zhCN_dat);

            while (dis_flag)
            {
            }; //等待中断刷完屏
            image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), ensure_fm, 24,
                                          (SIPEED_LCD_W - ((sizeof(ensure_fm)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 48,
                                          RED, NULL,
                                          SIPEED_LCD_W, SIPEED_LCD_H,
                                          lcd_dis_get_zhCN_dat);
            msleep(500);
            ret = 1;
            goto _exit;
        }

        /* 这里需要对固件进行大小端转换 */
        if (((fsize - 5) % 4) != 0)
        {
            printk("firmware len is not align 4bytes\r\n");

            while (dis_flag)
            {
            }; //等待中断刷完屏
            image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), fm_err, 24,
                                          (SIPEED_LCD_W - ((sizeof(fm_err)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 24,
                                          RED, NULL,
                                          SIPEED_LCD_W, SIPEED_LCD_H,
                                          lcd_dis_get_zhCN_dat);

            while (dis_flag)
            {
            }; //等待中断刷完屏
            image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), ensure_fm, 24,
                                          (SIPEED_LCD_W - ((sizeof(ensure_fm)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 48,
                                          RED, NULL,
                                          SIPEED_LCD_W, SIPEED_LCD_H,
                                          lcd_dis_get_zhCN_dat);
            msleep(500);
            ret = 1;
            goto _exit;
        }

        printk("update firmware...\r\n");

        /* 调整数据的大小端， 这里注意固件不是4字节对齐的，
        多一个字节，但是也需要进行一次转换，需要多申请3个字节的空间 */
        for (uint32_t i = 0; i < ((fsize - 4) / 4) + 1; i++)
        {
            tmp = *(file + 4 + i * 4 + 0);
            *(file + 4 + i * 4 + 0) = *(file + 4 + i * 4 + 3);
            *(file + 4 + i * 4 + 3) = tmp;

            tmp = *(file + 4 + i * 4 + 1);
            *(file + 4 + i * 4 + 1) = *(file + 4 + i * 4 + 2);
            *(file + 4 + i * 4 + 2) = tmp;
        }

        while (dis_flag)
        {
        }; //等待中断刷完屏
        image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), update_fm, 24,
                                      (SIPEED_LCD_W - ((sizeof(update_fm)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 24,
                                      WHITE, NULL,
                                      SIPEED_LCD_W, SIPEED_LCD_H,
                                      lcd_dis_get_zhCN_dat);
        while (dis_flag)
        {
        }; //等待中断刷完屏
        image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), about5min, 24,
                                      (SIPEED_LCD_W - ((sizeof(about5min)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 48,
                                      WHITE, NULL,
                                      SIPEED_LCD_W, SIPEED_LCD_H,
                                      lcd_dis_get_zhCN_dat);
        while (dis_flag)
        {
        }; //等待中断刷完屏
        image_rgb565_draw_zhCN_string(_IOMEM_PADDR(lcd_image), dontclose, 24,
                                      (SIPEED_LCD_W - ((sizeof(dontclose)) / 2 * 24)) / 2, (SIPEED_LCD_H - 24) / 2 + 72,
                                      RED, NULL,
                                      SIPEED_LCD_W, SIPEED_LCD_H,
                                      lcd_dis_get_zhCN_dat);

        msleep(500);

        uint64_t t = sysctl_get_time_us();
        w25qxx_status_t stat = w25qxx_write_data(0, (uint8_t *)(file + 4), fsize);

        printk("write use %ld us, stat:%d\r\n", sysctl_get_time_us() - t, stat);

        printk("update firmware over\r\n");

        msleep(1); /* 让打印能完全输出 */
        sysctl_reset(SYSCTL_RESET_SOC);
    }

_exit:
    if (file)
    {
        free(file);
    }

    return ret;
}

#if 0
/* clang-format off */
#define RGB565_RED          (0xf800)
#define RGB565_GREEN        (0x07e0)
#define RGB565_BLUE         (0x001f)
/* clang-format on */

// static uint8_t l_buf[3 * 1024];
static uint8_t l_buf[320 * 240 * 3];

extern char g_boot_time[32];

uint32_t record_cnt = 0;

uint8_t sd_save_img_ppm(char *fname, image_t *img, uint8_t r8g8b8)
{
    FIL fp;
    FRESULT res;

    uint32_t write_len, header_len = 0;

    char ppm_header[32];

    char ffname[64];
    sprintf(ffname, "%s/%s", g_boot_time, fname);

    printf("ffname:%s\r\n", ffname);

    if(0 != (res = f_open(&fs, &fp, ffname, FA_CREATE_ALWAYS | FA_WRITE | FA_READ)))
    {
        printk("%d open error!,res:%d\r\n", __LINE__, res);
        return 1; //open file error
    }

    header_len = sprintf(ppm_header, "P6\n%d %d\n255\n", img->width, img->height);
    if(0 != (res = f_write(&fp, ppm_header, header_len, &write_len)))
    {
        printk("%d write error!%d\r\n", __LINE__, res);
        goto end;
    }

    switch(img->pixel)
    {
        case 2: //RGB565
        {
            uint32_t total_len = img->width * img->height / 2;
            uint32_t per_write = 0, wrote_len = 0, temp_len = 0;
            uint16_t img_tmp = 0, *img_buf = (uint16_t *)(img->addr);

            printk("total_len:%d\r\n", total_len);

            do
            {
                per_write = ((total_len - wrote_len) > (512)) ? (512) : (total_len - wrote_len);
                temp_len = per_write * 6;
                // memset(l_buf, 0, sizeof(l_buf));
                for(uint32_t i = 0; i < temp_len; i += 6, img_buf += 2)
                {
                    img_tmp = (uint16_t) * (img_buf + 1);
                    l_buf[i + (3 * 0) + 0] = (uint8_t)((img_tmp & RGB565_RED) >> 8);
                    l_buf[i + (3 * 0) + 1] = (uint8_t)((img_tmp & RGB565_GREEN) >> 3);
                    l_buf[i + (3 * 0) + 2] = (uint8_t)((img_tmp & RGB565_BLUE) << 3);

                    img_tmp = (uint16_t) * (img_buf + 0);
                    l_buf[i + (3 * 1) + 0] = (uint8_t)((img_tmp & RGB565_RED) >> 8);
                    l_buf[i + (3 * 1) + 1] = (uint8_t)((img_tmp & RGB565_GREEN) >> 3);
                    l_buf[i + (3 * 1) + 2] = (uint8_t)((img_tmp & RGB565_BLUE) << 3);
                }

                uint64_t tim = sysctl_get_time_us();
                if(0 != (res = f_write(&fp, l_buf, temp_len, &write_len)))
                {
                    printk("%d write error! %d\r\n", __LINE__, res);
                    goto end;
                }

                printk("## %ld us\r\n", sysctl_get_time_us() - tim);
                if(write_len != (temp_len))
                {
                    printk("%d  write len error,write_len:%d, temp_len:%d\r\n", __LINE__, write_len, temp_len);
                    goto end;
                }

                wrote_len += per_write;
            } while(wrote_len < total_len);
        }
        break;
        case 3: //RGB888 or R8G8B8
        {
            if(r8g8b8)
            {
                uint32_t total_len = img->height * img->width;
                uint32_t temp_len = total_len * 3;

                // uint8_t *R_img_buf = (uint8_t *)(img->addr + total_len * 0);
                // uint8_t *G_img_buf = (uint8_t *)(img->addr + total_len * 1);
                // uint8_t *B_img_buf = (uint8_t *)(img->addr + total_len * 2);

                // for(uint32_t i = 0; i < total_len; i++)
                // {
                //     l_buf[i * 3 + 0] = *(R_img_buf + i);
                //     l_buf[i * 3 + 1] = *(G_img_buf + i);
                //     l_buf[i * 3 + 2] = *(B_img_buf + i);
                // }

                int w = img->width;
                int h = img->height;
                int oft = 0;

                // uint8_t *img_buf = _IOMEM_ADDR(img->addr);
                uint8_t *img_buf = (img->addr);

                for(int y = 0; y < h; y++)
                {
                    for(int x = 0; x < w; x++)
                    {
                        l_buf[oft + 0] = *(img_buf + y * w + x);
                        l_buf[oft + 1] = *(img_buf + y * w + x + w * h);
                        l_buf[oft + 2] = *(img_buf + y * w + x + w * h * 2);
                        oft += 3;
                    }
                }

                uint64_t tim = sysctl_get_time_us();
                if(0 != (res = f_write(&fp, l_buf, temp_len, &write_len)))
                {
                    printk("%d write error! %d\r\n", __LINE__, res);
                    goto end;
                }

                printk("## %ld us\r\n", sysctl_get_time_us() - tim);
                if(write_len != (temp_len))
                {
                    printk("%d  write len error,write_len:%d, temp_len:%d\r\n", __LINE__, write_len, temp_len);
                    goto end;
                }

                // uint32_t total_len = img->height * img->width;
                // uint32_t per_write = 0, wrote_len = 0, temp_len = 0;
                // uint8_t *img_buf = (uint8_t *)(img->addr);

                // do
                // {
                //     per_write = (total_len - wrote_len) > 1024 ? 1024 : (total_len - wrote_len);
                //     temp_len = per_write * 3;

                //     for(uint32_t i = 0; i < per_write; i += 3)
                //     {
                //         l_buf[i * 3 + 0] = *(img_buf + (total_len * 0) + wrote_len + i);
                //         l_buf[i * 3 + 1] = *(img_buf + (total_len * 1) + wrote_len + i);
                //         l_buf[i * 3 + 1] = *(img_buf + (total_len * 2) + wrote_len + i);
                //     }

                //     uint64_t tim = sysctl_get_time_us();
                //     if(0 != (res = f_write(&fp, l_buf, temp_len, &write_len)))
                //     {
                //         printk("%d write error! %d\r\n", __LINE__, res);
                //         goto end;
                //     }

                //     printk("## :%ld us\r\n", sysctl_get_time_us() - tim);
                //     if(write_len != (temp_len))
                //     {
                //         printk("%d  write len error,write_len:%d, temp_len:%d\r\n", __LINE__, write_len, temp_len);
                //         goto end;
                //     }

                //     wrote_len += per_write;
                // } while(wrote_len < total_len);

            } else
            {
                if(0 != (res = f_write(&fp, img->addr, (3 * img->height * img->width), &write_len)))
                {
                    printk("%d write error!%d\r\n", __LINE__, res);
                    goto end;
                }
                if(write_len != (3 * img->height * img->width))
                {
                    printk("%d  write len error\r\n", __LINE__);
                }
            }
        }
        break;
        default:
        {
            f_close(&fp);
            return 2; //unknown img pixel format
        }
        break;
    }
end:
    record_cnt++;

    // f_sync(&fp);
    f_close(&fp);

    return 0;
}
#endif
