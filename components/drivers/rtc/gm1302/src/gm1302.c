#include "gm1302.h"

#include "fpioa.h"
#include "gpiohs.h"
#include "sleep.h"

#include "global_config.h"
///////////////////////////////////////////////////////////////////////////////
/* clang-format off */
#define GPIOHS_OUT_HIGH(io) (*(volatile uint32_t *)0x3800100CU) |= (1 << (io))
#define GPIOHS_OUT_LOWX(io) (*(volatile uint32_t *)0x3800100CU) &= ~(1 << (io))

#define GET_GPIOHS_VALX(io) (((*(volatile uint32_t *)0x38001000U) >> (io)) & 1)

#define GM1302_DELAY_PER_BIT_US (100)

/* clang-format on */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void gm1302_enter_rw(void)
{
    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_GM1302_RST); //T_RST = 0;
    usleep(2);
    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK); //T_CLK = 0;
    usleep(2);
    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_GM1302_RST); //T_RST = 1;
    usleep(2);
}

static inline void gm1302_exit_rw(void)
{
    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK); //T_CLK = 1;
    usleep(2);
    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_GM1302_RST); //T_RST = 0;
    usleep(2);
}

static void gm1302_write_byte(uint8_t data)
{
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT, GPIO_DM_OUTPUT); /* output */
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x01) //LSB
        {
            GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT);
        }
        else
        {
            GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT);
        }
        data >>= 1;
        //CLK rise
        GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK);
        usleep(GM1302_DELAY_PER_BIT_US);
        GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK);
        usleep(GM1302_DELAY_PER_BIT_US);
    }
}

static uint8_t gm1302_read_byte(void)
{
    uint8_t tmp = 0;

    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT, GPIO_DM_INPUT); /* input */
    for (uint8_t i = 0; i < 8; i++)
    {
        if (GET_GPIOHS_VALX(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT))
        {
            tmp |= 0x80; //LSB
        }
        tmp >>= 1;
        //CLK fall
        GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK);
        usleep(GM1302_DELAY_PER_BIT_US);
        GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK);
        usleep(GM1302_DELAY_PER_BIT_US);
    }
    return tmp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void gm1302_write(uint8_t addr, uint8_t data)
{
    gm1302_enter_rw();
    gm1302_write_byte(addr);
    gm1302_write_byte(data);
    gm1302_exit_rw();
}

static uint8_t gm1302_read(uint8_t addr)
{
    uint8_t tmp = 0;
    gm1302_enter_rw();
    gm1302_write_byte(addr);
    tmp = gm1302_read_byte();
    gm1302_exit_rw();
    return tmp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static uint8_t calc_weekday(struct sipeed_rtc_time *time)
{
    uint8_t m = time->tm_mon;
    uint8_t d = time->tm_mday;

    int y = time->tm_year + RTC_BASE_YEAR;

    if (m == 1 || m == 2)
    {
        m += 12;
        y--;
    }
    return ((d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7) + 1;
}

static inline int time_in_range(uint8_t value, int min, int max)
{
    return ((value >= min) && (value <= max));
}

/* 这里我们使用`2019-10-12 15:29:30 6`这种时间格式,不支持其他格式. */
/* 年-月-日 时:分:秒 周 */
/* 1 error, 0 ok*/
static uint8_t gm1302_write_time(sipeed_rtc_t *rtc, struct sipeed_rtc_time *time)
{
    uint8_t weekday, year, high, low, tmp[8];

    if (time->tm_sec < 0 || time->tm_sec > 59)
    {
        return 1;
    }

    if (time->tm_min < 0 || time->tm_min > 59)
    {
        return 1;
    }

    if (time->tm_hour < 0 || time->tm_hour > 23)
    {
        return 1;
    }

    if (time->tm_mday < 1 || time->tm_mday > 31)
    {
        return 1;
    }

    if (time->tm_mon < 1 || time->tm_mon > 12)
    {
        return 1;
    }

    time->tm_year -= RTC_BASE_YEAR;
    if (time->tm_year < 0 || time->tm_year > 99)
    {
        return 1;
    }
    year = (uint8_t)(time->tm_year);

    weekday = calc_weekday(time);

    if (weekday < 1 || weekday > 7)
    {
        return 1;
    }

    time->tm_wday = weekday;
    ///////////////////////////////////////////////////////////////////////////////
    high = time->tm_sec / 10;
    low = time->tm_sec % 10;
    tmp[0] = (0 << 7) | ((high & 0x7) << 4) | (low & 0x0f); /* 秒,|CH.1|秒十位.3|秒个位.4|*/

    high = time->tm_min / 10;
    low = time->tm_min % 10;
    tmp[1] = (0 << 7) | ((high & 0x7) << 4) | (low & 0x0f); /* 分,|0.1|分十位.3|分个位.4|*/

    high = time->tm_hour / 10;
    low = time->tm_hour % 10;
    tmp[2] = ((high & 0x3) << 4) | (low & 0x0f); /* 时,|12/24.1|0.1|时十位.2|时个位.4|*/

    high = time->tm_mday / 10;
    low = time->tm_mday % 10;
    tmp[3] = ((high & 0x3) << 4) | (low & 0x0f); /* 日,|0.1|0.1|日十位.2|日个位.4|*/

    high = time->tm_mon / 10;
    low = time->tm_mon % 10;
    tmp[4] = ((high & 0x1) << 4) | (low & 0x0f); /* 月,|0.1|0.1|0.1|月十位.1|月个位.4|*/

    tmp[5] = (time->tm_wday & 0x0f); /* 周,|0.1|0.1|0.1|0.1|周.4|*/

    high = year / 10;
    low = year % 10;
    tmp[6] = ((high & 0x0f) << 4) | (low & 0x0f); /* 年,|年十位.4|年个位.4|*/
    tmp[7] = 0x80;
    ///////////////////////////////////////////////////////////////////////////////
    gm1302_write(GM1302_REG_WP, 0x00); /* 控制命令,WP=0,写操作*/

    gm1302_enter_rw();
    gm1302_write_byte(GM1302_REG_MULTIBYTE | GM1302_REG_WRITE); /* 0xbe:时钟多字节写命令 */
    for (uint8_t i = 0; i < 8; i++)                             /*8Byte = 7Byte 时钟数据 + 1Byte 控制*/
    {
        gm1302_write_byte(tmp[i]); /* 写 1Byte 数据*/
        // printf("%02X ", tmp[i]);
    }
    // printf("\r\n");
    gm1302_exit_rw();
    return 0;
}

static uint8_t gm1302_read_time(sipeed_rtc_t *rtc, struct sipeed_rtc_time *time)
{
    uint8_t tmp[7], t;

    gm1302_enter_rw();
    gm1302_write_byte(GM1302_REG_MULTIBYTE | GM1302_REG_READ); /* 0xbf:时钟多字节读命令 */
    for (uint8_t i = 0; i < 7; i++)
    {
        tmp[i] = gm1302_read_byte();
        // t = ((tmp[i] & 0xf0) >> 4) * 10 + (tmp[i] & 0x0f);
        // printf("%d ", i == 6 ? t + RTC_BASE_YEAR : t);
    }
    // printf("\r\n");

    gm1302_exit_rw();

    time->tm_sec = ((tmp[0] & 0x70) >> 4) * 10 + (tmp[0] & 0x0f); /* 秒,|CH.1|秒十位.3|秒个位.4|*/
    time->tm_min = ((tmp[1] & 0x70) >> 4) * 10 + (tmp[1] & 0x0f); /* 分,|0.1|分十位.3|分个位.4|*/

    if (tmp[2] & 0x80)
    {
        if ((tmp[2] & 0x20)) /* PM */
        {
            t = (tmp[2] & 0x10) * 10 + (tmp[2] & 0x0f);
            t += 12;
            time->tm_hour = t;
            if (t == 24)
            {
                time->tm_hour = 0;
            }
        }
        else /* AM */
        {
            time->tm_hour = (tmp[2] & 0x10) * 10 + (tmp[2] & 0x0f);
        }
    }
    else
    {
        time->tm_hour = ((tmp[2] & 0x30) >> 4) * 10 + (tmp[2] & 0x0f); /* 时,|12/24.1|0.1|时十位.2|时个位.4|*/
    }

    time->tm_mday = ((tmp[3] & 0x30) >> 4) * 10 + (tmp[3] & 0x0f); /* 日,|0.1|0.1|日十位.2|日个位.4|*/
    time->tm_mon = ((tmp[4] & 0x10) >> 4) * 10 + (tmp[4] & 0x0f);  /* 月,|0.1|0.1|0.1|月十位.1|月个位.4|*/

    time->tm_wday = (tmp[5] & 0x0f) - 1; /* 周,|0.1|0.1|0.1|0.1|周.4|; `- 1` to range (1,7) to (0,6)*/

    time->tm_year = ((tmp[6] & 0xf0) >> 4) * 10 + (tmp[6] & 0x0f) + RTC_BASE_YEAR; /* 年,|年十位.4|年个位.4|*/

    return 0;
}

/* I = (VCC - diode_num * 0.7) / res_val */
/* diode_num: 1,2; 0,disable */
/* res_val: 2K,4K,8K; 0,disable */
uint8_t gm1302_set_charge(uint8_t diode_num, uint8_t res_val)
{
    if (diode_num != 0 && diode_num != 1 && diode_num != 2)
    {
        return 1;
    }
    if (res_val != 0 && res_val != 2000 && res_val != 4000 && res_val != 8000)
    {
        return 1;
    }

    uint8_t reg_val = 0;

    if (res_val == 0)
    {
        reg_val = 0;
    }
    else
    {
        reg_val = 0xA0;

        if (diode_num == 1)
        {
            reg_val |= (0x1 << 2);
        }
        else if (diode_num == 2)
        {
            reg_val |= (0x2 << 2);
        }
        else
        {
            reg_val |= (0x0 << 2);
        }

        if (res_val == 2000)
        {
            reg_val |= 0x1;
        }
        else if (res_val == 4000)
        {
            reg_val |= 0x2;
        }
        else
        {
            reg_val |= 0x3;
        }
    }
    gm1302_write(GM1302_REG_WP | GM1302_REG_WRITE, 0x00); /* 控制命令,WP=0,写操作*/
    gm1302_write(GM1302_REG_CHARGE | GM1302_REG_WRITE, reg_val);
    gm1302_write(GM1302_REG_WP | GM1302_REG_WRITE, 0x80); /* 控制命令,WP=0,写操作*/
    return 0;
}

void gm1302_enable_crystal(void)
{
    uint8_t tmp;

    tmp = gm1302_read(GM1302_REG_SEC | GM1302_REG_READ);
    if ((tmp & 0x80) == 0)
    {
        gm1302_write(GM1302_REG_WP | GM1302_REG_WRITE, 0x00); /* 控制命令,WP=0,写操作*/
        gm1302_write(GM1302_REG_SEC | GM1302_REG_WRITE, tmp | 0x00);
        gm1302_write(GM1302_REG_WP | GM1302_REG_WRITE, 0x80); /* 控制命令,WP=0,写操作*/
    }
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* gm1302 ram op, not support now */
uint8_t gm1302_ram_write(uint8_t data[31], uint8_t start, uint8_t len)
{
    return 0;
}

uint8_t gm1302_ram_read(uint8_t data[31], uint8_t start, uint8_t len)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static uint8_t gm1302_config(void)
{
    fpioa_set_function(CONFIG_PIN_NUM_RTC_GM1302_RST, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RTC_GM1302_RST);
    fpioa_set_function(CONFIG_PIN_NUM_RTC_GM1302_CLK, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RTC_GM1302_CLK);
    fpioa_set_function(CONFIG_PIN_NUM_RTC_GM1302_DAT, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RTC_GM1302_DAT);

    //rst
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_GM1302_RST, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RTC_GM1302_RST, 0); /* default off */

    //clk
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RTC_GM1302_CLK, 0);

    //dat
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT, GPIO_DM_OUTPUT); /* output or input */
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RTC_GM1302_DAT, 1);

    /* 打开充电 */
    gm1302_set_charge(2, 8000);

    /* 打开晶振 */
    gm1302_enable_crystal();

    // struct tm time;
    // time.tm_sec = 0;
    // time.tm_min = 0;
    // time.tm_hour = 0;
    // time.tm_mday = 12;
    // time.tm_mon = 10;
    // time.tm_year = 2019;
    // gm1302_write_time(&time);

    return 0;
}

uint8_t rtc_gm1302_init(sipeed_rtc_t *rtc)
{
    rtc->config = gm1302_config;
    rtc->read_time = gm1302_read_time;
    rtc->write_time = gm1302_write_time;

    return 0;
}
