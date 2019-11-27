#ifndef __GM1302_H
#define __GM1302_H

#include <stdint.h>
#include <stdio.h>

#include "sipeed_rtc.h"

/* read: REG|0x01, write: REG|0x00 */
typedef enum gm1302_reg
{
    GM1302_REG_WRITE = 0x0,
    GM1302_REG_READ = 0x1,

    GM1302_REG_SEC = 0x80,
    GM1302_REG_MIN = 0x80 | (1 << 1),
    GM1302_REG_HOUR = 0x80 | (2 << 1),
    GM1302_REG_MDAY = 0x80 | (3 << 1),
    GM1302_REG_MON = 0x80 | (4 << 1),
    GM1302_REG_YEAR = 0x80 | (5 << 1),
    GM1302_REG_WDAY = 0x80 | (6 << 1),
    GM1302_REG_WP = 0x80 | (7 << 1),
    GM1302_REG_CHARGE = 0x80 | (8 << 1),
    GM1302_REG_MULTIBYTE = 0x80 | (0x1F << 1),
};

void gm1302_enable_crystal(void);

uint8_t gm1302_set_charge(uint8_t diode_num, uint8_t res_val);

uint8_t rtc_gm1302_init(sipeed_rtc_t *rtc);

#endif /* __GM1302_H */