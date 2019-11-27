#include "audio.h"

#include "printf.h"

#include "music_play.h"
#include "audio_speaker.h"

#include "global_config.h"

static audio_t _audio;

uint8_t audio_play(uint8_t *audio_buf, uint32_t audio_len)
{
    if (_audio.play)
    {
        return _audio.play(&_audio, audio_buf, audio_len);
    }
    return 0xff;
}

uint8_t audio_init(audio_type type)
{
    switch (type)
    {
#if CONFIG_SPK_TYPE_ES8374
    case AUDIO_TYPE_ES8374:
    {
        printk("AUDIO_TYPE_ES8374\r\n");
        audio_es8374_init(&_audio);
    }
    break;
#endif /* CONFIG_SPK_TYPE_ES8374 */
#if CONFIG_SPK_TYPE_PT8211
    case AUDIO_TYPE_PT8211:
    {
        printk("AUDIO_TYPE_PT8211\r\n");
        audio_pt8211_init(&_audio);
    }
    break;
#endif /* CONFIG_SPK_TYPE_PT8211 */
    default:
        printk("AUDIO_TYPE_UNK\r\n");
        break;
    }

    if (_audio.config)
    {
        return _audio.config();
    }

    return 0xff;
}