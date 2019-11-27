#include "sipeed_rtc.h"

#include "bm8563.h"
#include "gm1302.h"

#include "printf.h"

#include "global_config.h"

static sipeed_rtc_t _sipeed_rtc = {
    .config = NULL,
    .read_time = NULL,
    .write_time = NULL,
};

uint8_t sipeed_rtc_read_time(struct sipeed_rtc_time *time)
{
    if (_sipeed_rtc.read_time)
    {
        return _sipeed_rtc.read_time(&_sipeed_rtc, time);
    }
    return 0xFF;
}

uint8_t sipeed_rtc_write_time(struct sipeed_rtc_time *time)
{
    if (_sipeed_rtc.write_time)
    {
        return _sipeed_rtc.write_time(&_sipeed_rtc, time);
    }
    return 0xFF;
}

uint8_t sipeed_rtc_init(rtc_type type)
{
    switch (type)
    {
#if CONFIG_RTC_TYPE_GM1302
    case RTC_TYPE_GM1302:
    {
        printk("RTC_TYPE_GM1302\r\n");
        rtc_gm1302_init(&_sipeed_rtc);
    }
    break;
#endif /* CONFIG_RTC_TYPE_GM1302 */
#if CONFIG_RTC_TYPE_BM8563
    case RTC_TYPE_BM8563:
    {
        printk("RTC_TYPE_BM8563\r\n");
        rtc_bm8563_init(&_sipeed_rtc);
    }
    break;
#endif /* CONFIG_RTC_TYPE_BM8563 */
    default:
        printk("RTC_TYPE_UNK\r\n");
        break;
    }

    if (_sipeed_rtc.config)
    {
        return _sipeed_rtc.config();
    }
    return 0xFF;
}
