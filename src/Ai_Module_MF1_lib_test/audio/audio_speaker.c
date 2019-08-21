#include "audio_speaker.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "i2s.h"
#include "timer.h"
#include "dmac.h"
#include "stdio.h"
#include "sysctl.h"


audio_speaker_t audio_speaker;
uint16_t zero_buf[SPEAKER_FRAME_LENGTH];
uint32_t i2s_tx_buf[SPEAKER_FRAME_LENGTH * 2];

static void audio_speaker_io_mux_init()
{
    // I2S OUT BUZZER
    fpioa_set_function(SPEAKER_I2S_OUT_D1, FUNC_I2S2_OUT_D1);
    fpioa_set_function(SPEAKER_I2S_SCLK, FUNC_I2S2_SCLK);
    fpioa_set_function(SPEAKER_I2S_WS, FUNC_I2S2_WS);
    // PA
    fpioa_set_function(SPEAKER_PA_PIN, FUNC_GPIOHS0 + SPEAKER_PA_IO_NUM);
    gpiohs_set_drive_mode(SPEAKER_PA_IO_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(SPEAKER_PA_IO_NUM, GPIO_PV_HIGH);
}

static void i2s_parse_voice(audio_speaker_t *speaker, i2s_device_number_t device_num, uint32_t *buf, const uint8_t *pcm, size_t length, size_t bits_per_sample,
                            uint8_t track_num, size_t *send_len)
{
    uint32_t i, j = 0;
    *send_len = length * 2;
    switch(bits_per_sample)
    {
        case 16:
            for(i = 0; i < length; i++)
            {
                buf[2 * i] = (uint16_t)(((uint16_t *)pcm)[i] * speaker->volume);
                buf[2 * i + 1] = 0;
            }
            break;
        case 24:
            for(i = 0; i < length; i++)
            {
                buf[2 * i] = 0;
                buf[2 * i] |= pcm[j++];
                buf[2 * i] |= pcm[j++] << 8;
                buf[2 * i] |= pcm[j++] << 16;
                buf[2 * i + 1] = 0;
                if(track_num == 2)
                {
                    buf[2 * i + 1] |= pcm[j++];
                    buf[2 * i + 1] |= pcm[j++] << 8;
                    buf[2 * i + 1] |= pcm[j++] << 16;
                }
            }
            break;
        case 32:
        default:
            for(i = 0; i < length; i++)
            {
                buf[2 * i] = ((uint32_t *)pcm)[i];
                buf[2 * i + 1] = 0;
            }
            break;
    }
}

static int audio_speaker_timer_tx_irq(void *ctx)
{
    audio_speaker_t *speaker = (audio_speaker_t *)ctx;
    uint8_t *trans_buf;
    static int16_t *playing_buf = NULL; 
    size_t send_len = speaker->play_frame_length;
    static uint32_t timing_buf_count = 0;
    i2s_set_dma_divide_16(speaker->i2s_device_num, 0);
    if (speaker->play_state == PLAY_STATE_PLAYING)
    {
        speaker->ispaly = 1;
        if (speaker->play_buffer == NULL){
            trans_buf = (uint8_t *)zero_buf;
        }else if(playing_buf == speaker->play_buffer){
            trans_buf = (uint8_t *)(playing_buf + (speaker->current_frame * send_len));
        }else{
            speaker->current_frame = 0;
            playing_buf = speaker->play_buffer;
            trans_buf = (uint8_t *)(playing_buf + (speaker->current_frame * send_len));
        }

        i2s_parse_voice(speaker, speaker->i2s_device_num, i2s_tx_buf, trans_buf, speaker->play_frame_length, 16, 1, &send_len);
        i2s_send_data_dma(speaker->i2s_device_num, i2s_tx_buf, send_len, speaker->dmac_channel);
        speaker->current_frame += 1;
        switch(speaker->play_mode)
        {
            case PLAY_MODE_LOOP:
                if(speaker->current_frame >= speaker->play_buffer_frames)
                    speaker->current_frame = 0;
                break;
            case PLAY_MODE_ONCE:
                if(speaker->current_frame >= speaker->play_buffer_frames)
                {
                    speaker->current_frame = 0;
                    speaker->play_state = PLAY_STATE_STOP;
                }
                break;
            case PLAY_MODE_TIMING:
                timing_buf_count += 1;
                if(timing_buf_count >= speaker->play_time_frame)
                {
                    timing_buf_count = 0;
                    speaker->current_frame = 0;
                    speaker->play_state = PLAY_STATE_STOP;
                }else if(speaker->current_frame >= speaker->play_buffer_frames)
                    speaker->current_frame = 0;
                break;
            default :
                break;
        }
    }else if(speaker->play_state == PLAY_STATE_PAUSE)
    {
        if(speaker->ispaly){
            trans_buf = (uint8_t *)zero_buf;
            i2s_parse_voice(speaker, speaker->i2s_device_num, i2s_tx_buf, trans_buf, speaker->play_frame_length, 16, 1, &send_len);
            i2s_send_data_dma(speaker->i2s_device_num, i2s_tx_buf, send_len, speaker->dmac_channel);
        }
    }else if(speaker->play_state == PLAY_STATE_STOP)
    {
        speaker->ispaly = 0;
        speaker->current_frame = 0;

    }else{
        speaker->ispaly = 0;
        speaker->current_frame = 0;
    }
    return 0;
}

void audio_speaker_init(audio_speaker_t *speaker)
{
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
    audio_speaker_io_mux_init();
    
    speaker->ispaly = 0;
    speaker->volume = 1;

    speaker->i2s_device_num = SPEAKER_I2S_DEVICE;
    speaker->i2s_channel_num = SPEAKER_I2S_CHANNEL;
    speaker->dmac_channel = SPEAKER_DMAC_CHANNEL;
    speaker->timer_device = SPEAKER_TIMER_DEVICE;
    speaker->timer_channel = SPEAKER_TIMER_CHANNEL;

    speaker->play_frame_length = SPEAKER_FRAME_LENGTH;
    speaker->sample_rate = SPEAKER_SAMPLE_RATE;
    speaker->play_mode = PLAY_MODE_ONCE;
    speaker->play_state = PLAY_STATE_STOP;
    speaker->play_buffer = NULL;

    i2s_init(speaker->i2s_device_num, I2S_TRANSMITTER, 0xC);
    i2s_tx_channel_config(speaker->i2s_device_num, speaker->i2s_channel_num,
        RESOLUTION_16_BIT, SCLK_CYCLES_32,
        TRIGGER_LEVEL_4, RIGHT_JUSTIFYING_MODE);
    i2s_set_sample_rate(speaker->i2s_device_num, speaker->sample_rate);

    dmac_init();

    timer_init(speaker->timer_device);
    timer_irq_register(speaker->timer_device, speaker->timer_channel, 0, 7, audio_speaker_timer_tx_irq, speaker);
    size_t timer_time = (int16_t)((float)speaker->play_frame_length / (float)speaker->sample_rate * 1000) * 1000000 - 3000;
    timer_set_interval(speaker->timer_device, speaker->timer_channel, timer_time);
    timer_set_enable(speaker->timer_device, speaker->timer_channel, 1);
    
}
void audio_speaker_deinit(audio_speaker_t *speaker)
{
    timer_irq_unregister(speaker->timer_device, speaker->timer_channel);
    timer_set_enable(speaker->timer_device, speaker->timer_channel, 0);
    gpiohs_set_pin(SPEAKER_PA_IO_NUM, GPIO_PV_LOW);
}

void audio_speaker_set_sample_rate(audio_speaker_t *speaker, uint32_t sample_rate)
{
    speaker->sample_rate = sample_rate;
    i2s_set_sample_rate(speaker->i2s_device_num, speaker->sample_rate);
}
void audio_speaker_set_mode(audio_speaker_t *speaker, audio_play_mode_t mode)
{
    speaker->play_mode = mode;
}
void audio_speaker_set_timing_mode_time(audio_speaker_t *speaker, uint32_t ms)
{
    speaker->play_time_ms = ms;
    speaker->play_time_frame = (uint32_t)((float)speaker->sample_rate / 1000.f * ms) / speaker->play_frame_length; 
}
void audio_speaker_set_buffer(audio_speaker_t *speaker, int16_t* buf, uint64_t length)
{
    speaker->play_buffer = buf;
    speaker->play_buffer_length = length;
    speaker->play_buffer_frames = speaker->play_buffer_length / speaker->play_frame_length;
}
void audio_speaker_set_volume(audio_speaker_t *speaker, uint32_t level)
{
    if(level > 200)
        level = 200;
    else if (level < 0)
        level = 0;
    speaker->volume = level / 100.f;
}

uint32_t audio_speaker_get_sample_rate(audio_speaker_t *speaker)
{
    return speaker->sample_rate;
}
audio_play_mode_t audio_speaker_get_mode(audio_speaker_t *speaker)
{
    return speaker->play_mode;
}
audio_play_state_t audio_speaker_get_state(audio_speaker_t *speaker)
{
    return speaker->play_state;
}
uint32_t audio_speaker_get_timing_mode_time(audio_speaker_t *speaker)
{
    return speaker->play_time_ms;
}
float audio_speaker_get_volume(audio_speaker_t *speaker)
{
    return speaker->volume;
}

void audio_speaker_play(audio_speaker_t *speaker)
{
    speaker->play_state = PLAY_STATE_PLAYING;
}
void audio_speaker_pause(audio_speaker_t *speaker)
{
    speaker->play_state = PLAY_STATE_PAUSE;
}
void audio_speaker_resume(audio_speaker_t *speaker)
{
    if(speaker->ispaly)
    {
        speaker->play_state = PLAY_STATE_PLAYING;
    }
}
void audio_speaker_stop(audio_speaker_t *speaker)
{
    speaker->play_state = PLAY_STATE_STOP;
}
void audio_speaker_replay(audio_speaker_t *speaker)
{
    if(speaker->ispaly)
    {
        speaker->current_frame = 0;
        speaker->play_state = PLAY_STATE_PLAYING;
    }
}
