#include "bm8563.h"

#include "fpioa.h"
#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"
#include "printf.h"

#include "global_config.h"

///////////////////////////////////////////////////////////////////////////////
#define GPIOHS_OUT_HIGH(io) (*(volatile uint32_t *)0x3800100CU) |= (1 << (io))
#define GPIOHS_OUT_LOWX(io) (*(volatile uint32_t *)0x3800100CU) &= ~(1 << (io))

#define GET_GPIOHS_VALX(io) (((*(volatile uint32_t *)0x38001000U) >> (io)) & 1)

#define BM8563_DELAY_PER_BIT_US (2)

#define BM8563_I2C_ADDR (0xA2)

#define DEBUG (1)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void i2c_start(void)
{
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_OUTPUT);

    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    usleep(BM8563_DELAY_PER_BIT_US);
    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);

    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    usleep(BM8563_DELAY_PER_BIT_US);
    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);
}

static void i2c_stop(void)
{
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_OUTPUT);

    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);
    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    usleep(BM8563_DELAY_PER_BIT_US);

    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);
    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    usleep(BM8563_DELAY_PER_BIT_US);
}

static void i2c_ack(uint8_t ack)
{
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_OUTPUT);

    if (ack)
    {
        GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    }
    else
    {
        GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    }

    usleep(BM8563_DELAY_PER_BIT_US);

    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);
    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);
}

static uint8_t i2c_wait_ack(void)
{
    uint8_t ucErrTime = 0;

    usleep(BM8563_DELAY_PER_BIT_US);
    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    usleep(BM8563_DELAY_PER_BIT_US);

    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);

    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_INPUT_PULL_DOWN);
    usleep(BM8563_DELAY_PER_BIT_US);
    while (GET_GPIOHS_VALX(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA))
    {
        usleep(BM8563_DELAY_PER_BIT_US);

        ucErrTime++;
        if (ucErrTime > 250)
        {
#if DEBUG
            printk("%s:%d\r\n", __func__, __LINE__);
#endif
            i2c_stop();
            return 1;
        }
    }

    GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    usleep(BM8563_DELAY_PER_BIT_US);

    return 0;
}

static void i2c_write_byte(uint8_t data)
{
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_OUTPUT);

    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
        }
        else
        {
            GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
        }
        data <<= 1;

        usleep(BM8563_DELAY_PER_BIT_US);
        GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
        usleep(BM8563_DELAY_PER_BIT_US);
        GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    }
}

static uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t receive = 0;

    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    usleep(BM8563_DELAY_PER_BIT_US);

    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_INPUT_PULL_DOWN);

    for (uint8_t i = 0; i < 8; i++)
    {
        GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
        usleep(BM8563_DELAY_PER_BIT_US);
        GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
        usleep(BM8563_DELAY_PER_BIT_US);

        receive <<= 1;
        if (GET_GPIOHS_VALX(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA))
        {
            receive++;
        }

        usleep(BM8563_DELAY_PER_BIT_US);
    }

    i2c_ack(ack);

    return receive;
}

///////////////////////////////////////////////////////////////////////////////
static void bm8563_write_byte(uint8_t addr, uint8_t data)
{
    i2c_start();

    i2c_write_byte(BM8563_I2C_ADDR);
    if (i2c_wait_ack())
    {
#if DEBUG
        printk("%s:%d\r\n", __func__, __LINE__);
#endif
        return;
    }

    i2c_write_byte(addr & 0xFF);
    if (i2c_wait_ack())
    {
#if DEBUG
        printk("%s:%d\r\n", __func__, __LINE__);
#endif
        return;
    }

    i2c_write_byte(data);
    if (i2c_wait_ack())
    {
#if DEBUG
        printk("%s:%d\r\n", __func__, __LINE__);
#endif
        return;
    }

    i2c_stop();
    return;
}

static uint8_t bm8563_read_byte(uint8_t addr)
{
    uint8_t data = 0xFF;

    i2c_start();

    i2c_write_byte(BM8563_I2C_ADDR);
    if (i2c_wait_ack())
    {
#if DEBUG
        printk("%s:%d\r\n", __func__, __LINE__);
#endif
        return 1;
    }

    i2c_write_byte(addr & 0xFF);
    if (i2c_wait_ack())
    {
#if DEBUG
        printk("%s:%d\r\n", __func__, __LINE__);
#endif
        return 1;
    }

    i2c_start();

    i2c_write_byte(BM8563_I2C_ADDR + 1);
    if (i2c_wait_ack())
    {
#if DEBUG
        printk("%s:%d\r\n", __func__, __LINE__);
#endif
        return 1;
    }

    data = i2c_read_byte(1);

    i2c_stop();

    return data;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* 0:OK,1:VL */
static uint8_t bm8563_read_time(sipeed_rtc_t *rtc, struct sipeed_rtc_time *time)
{
    uint8_t buf[7];
    uint8_t vl = 0, tmp;

    for (uint8_t i = 0; i < 7; i++)
    {
        buf[i] = bm8563_read_byte(0x02 + i);
    }

    vl = (buf[0] & 0x80) >> 7;

    tmp = buf[0] & 0x7f;
    time->tm_sec = ((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f);

    tmp = buf[1] & 0x7f;
    time->tm_min = ((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f);

    tmp = buf[2] & 0x3f;
    time->tm_hour = ((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f);

    tmp = buf[3] & 0x3f;
    time->tm_mday = ((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f);

    tmp = (buf[4] & 0x07) + 1; /* [0-6] --> [1-7] */
    time->tm_wday = ((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f);

    tmp = buf[5] & 0x1f;
    time->tm_mon = ((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f);

    tmp = (buf[6] & 0xff);
    time->tm_year = (((tmp & 0xf0) >> 4) * 10 + (tmp & 0x0f)) + ((buf[5] & 0x80) ? 1900 : 2000);

    return vl;
}

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
static uint8_t bm8563_write_time(sipeed_rtc_t *rtc, struct sipeed_rtc_time *time)
{
    uint8_t weekday, year, high, low, tmp[11];

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

    time->tm_wday = weekday - 1; /* BM8563 is diff to GM1302 */
    ///////////////////////////////////////////////////////////////////////////////
    high = time->tm_sec / 10;
    low = time->tm_sec % 10;
    tmp[0] = (0 << 7) | ((high & 0x7) << 4) | (low & 0x0f); /* 秒,|VL.1|秒十位.3|秒个位.4|*/

    high = time->tm_min / 10;
    low = time->tm_min % 10;
    tmp[1] = ((high & 0x7) << 4) | (low & 0x0f); /* 分,|0.1|分十位.3|分个位.4|*/

    high = time->tm_hour / 10;
    low = time->tm_hour % 10;
    tmp[2] = ((high & 0x3) << 4) | (low & 0x0f); /* 时,|0.2|时十位.2|时个位.4|*/

    high = time->tm_mday / 10;
    low = time->tm_mday % 10;
    tmp[3] = ((high & 0x3) << 4) | (low & 0x0f); /* 日,|0.1|0.1|日十位.2|日个位.4|*/

    tmp[4] = (time->tm_wday & 0x0f); /* 周,|0.1|0.1|0.1|0.1|周.4|*/

    high = time->tm_mon / 10;
    low = time->tm_mon % 10;
    tmp[5] = (0 << 7) | ((high & 0x1) << 4) | (low & 0x0f); /* 月,|0.1|0.1|0.1|月十位.1|月个位.4|*/

    high = year / 10;
    low = year % 10;
    tmp[6] = ((high & 0x0f) << 4) | (low & 0x0f); /* 年,|年十位.4|年个位.4|*/

    /* ALARM not use */
    tmp[7] = 0x80;
    tmp[8] = 0x80;
    tmp[9] = 0x80;
    tmp[10] = 0x80;
    ///////////////////////////////////////////////////////////////////////////////
    for (uint8_t i = 0; i < 11; i++)
    {
        bm8563_write_byte(0x02 + i, tmp[i]);
    }

    return 0;
}

static uint8_t bm8563_config(void)
{
    fpioa_set_function(CONFIG_PIN_NUM_RTC_BM8563_SDA, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    fpioa_set_function(CONFIG_PIN_NUM_RTC_BM8563_SCL, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);

    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA, GPIO_DM_OUTPUT);

    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SCL);
    GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_RTC_BM8563_SDA);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint8_t rtc_bm8563_init(sipeed_rtc_t *rtc)
{
    rtc->config = bm8563_config;
    rtc->read_time = bm8563_read_time;
    rtc->write_time = bm8563_write_time;

    return 0;
}
