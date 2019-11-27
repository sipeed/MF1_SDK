#ifndef __SIPEED_RTC_H
#define __SIPEED_RTC_H

#include <stdint.h>

#define RTC_BASE_YEAR (2000)

typedef enum
{
    RTC_TYPE_GM1302 = 0,
    RTC_TYPE_BM8563 = 1,
    RTC_TYPE_MAX,
} rtc_type;

struct sipeed_rtc_time
{
    uint8_t tm_sec;   /* 秒, 取值区间为[0,59] */
    uint8_t tm_min;   /* 分, 取值区间为[0,59] */
    uint8_t tm_hour;  /* 时, 取值区间为[01,12] / [0,23] */
    uint8_t tm_mday;  /* 日, 取值区间为[0,28 / 29] / [1,30] / [1,31] */
    uint8_t tm_mon;   /* 月, 取值区间为[1,12] */
    uint8_t tm_wday;  /* 周, 取值区间为[1,7] */
    uint16_t tm_year; /* 年, 取值区间为[00,99] */
};

typedef struct _sipeed_rtc sipeed_rtc_t;
typedef struct _sipeed_rtc
{
    uint8_t (*config)(void);

    uint8_t (*read_time)(sipeed_rtc_t *rtc, struct sipeed_rtc_time *time);

    uint8_t (*write_time)(sipeed_rtc_t *rtc, struct sipeed_rtc_time *time);

} sipeed_rtc_t;

uint8_t sipeed_rtc_init(rtc_type type);

uint8_t sipeed_rtc_read_time(struct sipeed_rtc_time *time);

uint8_t sipeed_rtc_write_time(struct sipeed_rtc_time *time);

#endif /* __SIPEED_RTC_H */
