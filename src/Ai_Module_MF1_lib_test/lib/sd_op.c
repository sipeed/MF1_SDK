#include "sd_op.h"

#include <stdio.h>

#include "sysctl.h"

#include "ff.h"
#include "sd_card.h"

FATFS fs;

void sd_init_fatfs(void)
{
    uint64_t tim = sysctl_get_time_us();
    uint8_t ret = SD_Initialize();
    printf("init sd us :%ld\r\n", sysctl_get_time_us() - tim);

    if(ret != 0)
    {
        printf("ret: 0x%02X\r\n", ret);
        return;
    }

    uint32_t sd_size = SD_GetSectorCount();
    printf("SD Type: %d Size: %dMB\r\n", SD_Type, sd_size >> 11);

    FRESULT res = f_mount(&fs);
    printf(res == FR_OK ? ""
                          "mount ok!\r\n"
                        : "mount error!%d\r\n",
           res);
    return;
}

/* clang-format off */
#define RGB565_RED          (0xf800)
#define RGB565_GREEN        (0x07e0)
#define RGB565_BLUE         (0x001f)
/* clang-format on */

uint8_t sd_save_img_ppm(char *fname, image_t *img)
{
    FIL fp;
    uint32_t write_len, header_len = 0;
    uint8_t l_buf[3 * 1024];
    char ppm_header[32];

    if(0 != f_open(&fs, &fp, fname, FA_CREATE_ALWAYS | FA_WRITE | FA_READ))
    {
        return 1; //open file error
    }

    header_len = sprintf(ppm_header, "P6\r\n%d %d\r\n255\r\n", img->width, img->height);
    if(0 != f_write(&fp, ppm_header, header_len, &write_len))
    {
        printf("%d write error!\r\n", __LINE__);
        goto end;
    }

    switch(img->pixel)
    {
        case 2: //RGB565
        {
            uint32_t total_len = 0;
            uint32_t rgb565_wrote_len = 0, rgb565_per_write_len = 0;
            uint32_t rgb888_wrote_len = 0, rgb888_per_write_len = 0;
            uint16_t *img_buf = (uint16_t *)(img->addr);

            total_len = (2 * img->height * img->width);
            do
            {
                rgb565_per_write_len = (total_len - rgb565_wrote_len) > (2 * 1024) ? (2 * 1024) : (total_len - rgb565_wrote_len);
                
                for(uint16_t i = 0; i < rgb565_per_write_len; i++)
                {
                    l_buf[rgb888_per_write_len + i * 3 + 0] = (*(img_buf)&RGB565_RED) >> 8;
                    l_buf[rgb888_per_write_len + i * 3 + 0] = (*(img_buf)&RGB565_GREEN) >> 3;
                    l_buf[rgb888_per_write_len + i * 3 + 0] = (*(img_buf)&RGB565_BLUE) << 3;
                    img_buf++;
                }
                rgb888_per_write_len = (rgb565_per_write_len / 2) * 3;

                if(0 != f_write(&fp, l_buf, rgb888_per_write_len, &write_len))
                {
                    printf("%d write error!\r\n", __LINE__);
                    goto end;
                }
                if(write_len != (rgb888_per_write_len))
                {
                    printf("%d  write len error\r\n", __LINE__);
                }

                rgb565_wrote_len += rgb565_per_write_len;
                rgb888_wrote_len += rgb888_per_write_len;

            } while(rgb565_wrote_len < total_len);
        }
        break;
        case 3: //RGB888
        {
            if(0 != f_write(&fp, img->addr, (3 * img->height * img->width), &write_len))
            {
                printf("%d write error!\r\n", __LINE__);
                goto end;
            }
            if(write_len != (3 * img->height * img->width))
            {
                printf("%d  write len error\r\n", __LINE__);
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
    f_close(&fp);
    return 0;
}
