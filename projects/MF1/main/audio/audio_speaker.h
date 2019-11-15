#ifndef __AUDIO_SPEAKER_H
#define __AUDIO_SPEAKER_H

#include <stdint.h>

#include "dmac.h"
#include "i2s.h"
#include "timer.h"

#define USE_ES8374 0

#if USE_ES8374
#include "es8374.h"
#include "maix_i2s.h"
#define SPEAKER_I2S_OUT    ES8374_I2S_DIN
#define SPEAKER_I2S_SCLK   ES8374_I2S_SCLK
#define SPEAKER_I2S_WS     ES8374_I2S_WS
#define SPEAKER_I2S_MCLK   ES8374_I2S_MCLK

#define SPEAKER_I2S_DEVICE  ES8374_I2S_DEVICE
#define SPEAKER_I2S_CHANNEL ES8374_TX_CHANNEL
#define SPEAKER_DMAC_CHANNEL ES8374_TX_DMAC_CHANNEL

#define SPEAKER_TIMER_DEVICE  TIMER_DEVICE_0
#define SPEAKER_TIMER_CHANNEL TIMER_CHANNEL_0

#else
#define SPEAKER_I2S_OUT_D1 34
#define SPEAKER_I2S_SCLK   35
#define SPEAKER_I2S_WS     33
#define SPEAKER_PA_PIN     17
#define SPEAKER_PA_IO_NUM  29

#define SPEAKER_I2S_DEVICE    I2S_DEVICE_2
#define SPEAKER_I2S_CHANNEL   I2S_CHANNEL_1
#define SPEAKER_DMAC_CHANNEL  DMAC_CHANNEL4
#define SPEAKER_TIMER_DEVICE  TIMER_DEVICE_0
#define SPEAKER_TIMER_CHANNEL TIMER_CHANNEL_0
#endif


#define SPEAKER_SAMPLE_RATE   (16000)
#define SPEAKER_FRAME_LENGTH  (512)

typedef enum _audio_play_mode_t{
    PLAY_MODE_ONCE,
    PLAY_MODE_LOOP,
    PLAY_MODE_TIMING
} audio_play_mode_t;

typedef enum _audio_play_state_t{
    PLAY_STATE_STOP,
    PLAY_STATE_PLAYING,
    PLAY_STATE_PAUSE,
} audio_play_state_t;

typedef enum _audio_play_volume_level_t{
    PLAY_VOLUME_LEVEL_0,
    PLAY_VOLUME_LEVEL_25,
    PLAY_VOLUME_LEVEL_50,
    PLAY_VOLUME_LEVEL_75,
    PLAY_VOLUME_LEVEL_100,
    PLAY_VOLUME_LEVEL_125,
    PLAY_VOLUME_LEVEL_150,
    PLAY_VOLUME_LEVEL_175,
    PLAY_VOLUME_LEVEL_200,
} audio_play_volume_level_t;

typedef struct _audio_speaker_t
{
    float volume;
    uint8_t ispaly;
    audio_play_mode_t play_mode;
    audio_play_state_t play_state;

    uint32_t sample_rate;
    uint16_t play_frame_length;
    uint16_t current_frame;
    int16_t* play_buffer;
    uint64_t play_buffer_length;
    uint16_t play_buffer_frames;
    uint32_t play_time_ms;
    uint32_t play_time_frame;

    i2s_device_number_t i2s_device_num;
    i2s_channel_num_t   i2s_channel_num;
    dmac_channel_number_t dmac_channel;
    timer_device_number_t timer_device;
    timer_channel_number_t timer_channel;

} audio_speaker_t;

extern audio_speaker_t audio_speaker;

void audio_speaker_init();
void audio_speaker_deinit();

void audio_speaker_set_sample_rate(uint32_t sample_rate);
void audio_speaker_set_mode(audio_play_mode_t mode);
void audio_speaker_set_timing_mode_time(uint32_t ms);
void audio_speaker_set_buffer(int16_t* buf, uint64_t length);
void audio_speaker_set_volume(audio_play_volume_level_t level);

uint32_t audio_speaker_get_sample_rate();
audio_play_mode_t audio_speaker_get_mode();
audio_play_state_t audio_speaker_get_state();
uint32_t audio_speaker_get_timing_mode_time();
float audio_speaker_get_volume();

void audio_speaker_play();
void audio_speaker_pause();
void audio_speaker_resume();
void audio_speaker_stop();
void audio_speaker_replay();

int16_t *audio_speaker_read_from_flash(uint32_t addr, uint32_t length);
void audio_speaker_free_buf(int16_t *buf);

#endif