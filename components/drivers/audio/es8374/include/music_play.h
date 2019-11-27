#ifndef __MUSIC_PLAY_H
#define __MUSIC_PLAY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "i2s.h"

#include "audio.h"

/* clang-format off */
// #define ES8374_I2C_SDA    (31)
// #define ES8374_I2C_SCL    (30)

// #define ES8374_I2S_DIN    (18)
// #define ES8374_I2S_MCLK   (19)

// #define ES8374_I2S_WS     (33)
// #define ES8374_I2S_DOUT   (34)
// #define ES8374_I2S_SCLK   (35)
/* clang-format on */

void maix_i2s_init(i2s_device_number_t device_num, uint32_t rx_channel_mask, uint32_t tx_channel_mask);

void maix_i2s_play(i2s_device_number_t device_num, dmac_channel_number_t channel_num,
                   const uint8_t *buf, size_t buf_len, size_t frame, size_t bits_per_sample, uint8_t track_num);

uint8_t music_play_init(void);

uint8_t audio_es8374_init(audio_t *audio);

#endif /* __MUSIC_PLAY_H */
