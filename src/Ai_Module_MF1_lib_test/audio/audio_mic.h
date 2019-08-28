#ifndef __AUDIO_MIC_H
#define __AUDIO_MIC_H

#include <stdint.h>

#include "dmac.h"
#include "i2s.h"

#define MIC_I2S_IN_D0 20
#define MIC_I2S_WS    19
#define MIC_I2S_SCLK  18

#define MIC_I2S_DEVICE    I2S_DEVICE_0
#define MIC_I2S_CHANNEL   I2S_CHANNEL_0
#define MIC_DMAC_CHANNEL  DMAC_CHANNEL3

#define MIC_SAMPLE_RATE   (16000)
#define MIC_FRAME_LENGTH  (512)

typedef enum _audio_recv_mode_t{
    RECV_MODE_ALWAYS,
    RECV_MODE_TIMING,
} audio_recv_mode_t;

typedef enum _audio_recv_state_t{
    RECV_STATE_IDLE,
    RECV_STATE_RECVING,
    RECV_STATE_READY,
} audio_recv_state_t;

typedef struct _audio_mic_t
{
    audio_recv_mode_t recv_mode;
    audio_recv_state_t recv_state;
    uint8_t mic_recv_buf_index;

    uint32_t sample_rate;
    uint16_t recv_frame_length;
    int16_t *recv_buffer;
    uint64_t recv_buffer_length;
    uint16_t recv_buffer_frames;
    uint32_t recv_time_ms;
    uint32_t recv_time_frames;

    i2s_device_number_t i2s_device_num;
    i2s_channel_num_t   i2s_channel_num;
    dmac_channel_number_t dmac_channel;
} audio_mic_t;

extern audio_mic_t audio_mic;

void audio_mic_init();
void audio_mic_deinit();

void audio_mic_set_sample_rate(uint32_t sample_rate);
void audio_mic_set_mode(audio_recv_mode_t mode);
void audio_mic_set_timing_mode_time(uint32_t ms);
void audio_mic_set_buffer(int16_t *buf, uint64_t length);
void audio_mic_set_flash_address(uint32_t address, uint64_t length);

uint32_t audio_mic_get_sample_rate();
audio_recv_mode_t audio_mic_get_mode();
audio_recv_state_t audio_mic_get_state();
uint32_t audio_mic_get_timing_mode_time();

void audio_mic_start();
void audio_mic_stop();
void audio_mic_clear();

void audio_mic_save_to_flash(uint32_t addr, int16_t* save_buf, uint32_t length);


#endif