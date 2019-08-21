#ifndef __AUDIO_SPEAKER_H
#define __AUDIO_SPEAKER_H

#include <stdint.h>

#include "dmac.h"
#include "i2s.h"
#include "timer.h"

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

void audio_speaker_init(audio_speaker_t *speaker);
void audio_speaker_deinit(audio_speaker_t *speaker);

void audio_speaker_set_sample_rate(audio_speaker_t *speaker, uint32_t sample_rate);
void audio_speaker_set_mode(audio_speaker_t *speaker, audio_play_mode_t mode);
void audio_speaker_set_timing_mode_time(audio_speaker_t *speaker, uint32_t ms);
void audio_speaker_set_buffer(audio_speaker_t *speaker, int16_t* buf, uint64_t length);
void audio_speaker_set_volume(audio_speaker_t *speaker, uint32_t level);

uint32_t audio_speaker_get_sample_rate(audio_speaker_t *speaker);
audio_play_mode_t audio_speaker_get_mode(audio_speaker_t *speaker);
audio_play_state_t audio_speaker_get_state(audio_speaker_t *speaker);
uint32_t audio_speaker_get_timing_mode_time(audio_speaker_t *speaker);
float audio_speaker_get_volume(audio_speaker_t *speaker);

void audio_speaker_play(audio_speaker_t *speaker);
void audio_speaker_pause(audio_speaker_t *speaker);
void audio_speaker_resume(audio_speaker_t *speaker);
void audio_speaker_stop(audio_speaker_t *speaker);
void audio_speaker_replay(audio_speaker_t *speaker);

#endif