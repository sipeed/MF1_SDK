#include "music_play.h"

#include "sysctl.h"
#include "fpioa.h"

#include "es8374.h"

#include "global_config.h"

static void i2s_parse_voice(i2s_device_number_t device_num, uint32_t *buf, const uint8_t *pcm, size_t length, size_t bits_per_sample,
                            uint8_t track_num, size_t *send_len)
{
    uint32_t i, j = 0;
    *send_len = length * 2;
    switch (bits_per_sample)
    {
    case 16:
        for (i = 0; i < length; i++)
        {
            buf[2 * i] = ((uint16_t *)pcm)[i];
            //buf[2 * i + 1] = buf[2 * i];
            buf[2 * i + 1] = 0;
        }
        break;
    case 24:
        for (i = 0; i < length; i++)
        {
            buf[2 * i] = 0;
            buf[2 * i] |= pcm[j++];
            buf[2 * i] |= pcm[j++] << 8;
            buf[2 * i] |= pcm[j++] << 16;
            buf[2 * i + 1] = 0;
            if (track_num == 2)
            {
                buf[2 * i + 1] |= pcm[j++];
                buf[2 * i + 1] |= pcm[j++] << 8;
                buf[2 * i + 1] |= pcm[j++] << 16;
            }
        }
        break;
    case 32:
    default:
        for (i = 0; i < length; i++)
        {
            buf[2 * i] = ((uint32_t *)pcm)[i];
            buf[2 * i + 1] = 0;
        }
        break;
    }
}

void maix_i2s_play(i2s_device_number_t device_num, dmac_channel_number_t channel_num,
                   const uint8_t *buf, size_t buf_len, size_t frame, size_t bits_per_sample, uint8_t track_num)
{
    const uint8_t *trans_buf;
    uint32_t i;
    size_t sample_cnt = buf_len / (bits_per_sample / 8) / track_num;
    size_t frame_cnt = sample_cnt / frame;
    size_t frame_remain = sample_cnt % frame;
    i2s_set_dma_divide_16(device_num, 0);

    if (bits_per_sample == 16 && track_num == 2)
    {
        i2s_set_dma_divide_16(device_num, 1);
        for (i = 0; i < frame_cnt; i++)
        {
            trans_buf = buf + i * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num, trans_buf, frame, channel_num);
        }
        if (frame_remain)
        {
            trans_buf = buf + frame_cnt * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num, trans_buf, frame_remain, channel_num);
        }
    }
    else if (bits_per_sample == 32 && track_num == 2)
    {
        for (i = 0; i < frame_cnt; i++)
        {
            trans_buf = buf + i * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num, trans_buf, frame * 2, channel_num);
        }
        if (frame_remain)
        {
            trans_buf = buf + frame_cnt * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num, trans_buf, frame_remain * 2, channel_num);
        }
    }
    else
    {
        uint32_t *buff[2];
        buff[0] = malloc(frame * 2 * sizeof(uint32_t) * 2);
        buff[1] = buff[0] + frame * 2;
        uint8_t flag = 0;
        size_t send_len = 0;
        for (i = 0; i < frame_cnt; i++)
        {
            trans_buf = buf + i * frame * (bits_per_sample / 8) * track_num;
            i2s_parse_voice(device_num, buff[flag], trans_buf, frame, bits_per_sample, track_num, &send_len);
            i2s_send_data_dma(device_num, buff[flag], send_len, channel_num);
            flag = !flag;
        }
        if (frame_remain)
        {
            trans_buf = buf + frame_cnt * frame * (bits_per_sample / 8) * track_num;
            i2s_parse_voice(device_num, buff[flag], trans_buf, frame_remain, bits_per_sample, track_num, &send_len);
            i2s_send_data_dma(device_num, trans_buf, send_len, channel_num);
        }
        free(buff[0]);
    }
}

void i2s_io_mux_init()
{
    fpioa_set_function(CONFIG_PIN_NUM_ES8374_MCLK, FUNC_I2S0_MCLK + 11 * ES8374_I2S_DEVICE);
    //fpioa_set_function(34, FUNC_I2S0_MCLK);
    fpioa_set_function(CONFIG_PIN_NUM_ES8374_SCLK, FUNC_I2S0_SCLK + 11 * ES8374_I2S_DEVICE);
    fpioa_set_function(CONFIG_PIN_NUM_ES8374_WS, FUNC_I2S0_WS + 11 * ES8374_I2S_DEVICE);
    fpioa_set_function(CONFIG_PIN_NUM_ES8374_DOUT, FUNC_I2S0_IN_D0 + 11 * ES8374_I2S_DEVICE);
    fpioa_set_function(CONFIG_PIN_NUM_ES8374_DIN, FUNC_I2S0_OUT_D2 + 11 * ES8374_I2S_DEVICE);
}

uint8_t music_play_init(void)
{
    ///////////////////////////////////////////////////////////////////////////
    sysctl_pll_set_freq(SYSCTL_PLL2, 262144000UL);

    ///////////////////////////////////////////////////////////////////////////
    es8374_i2s_iface_t iface;
    iface.bits = ES8374_BIT_LENGTH_16BITS;
    iface.fmt = ES8374_I2S_NORMAL;
    iface.mode = ES8374_MODE_SLAVE;
    iface.samples = ES8374_16K_SAMPLES;

    es8374_config_t cfg;
    cfg.adc_input = ES8374_ADC_INPUT_LINE1;
    cfg.dac_output = ES8374_DAC_OUTPUT_LINE1;
    cfg.es8374_mode = ES8374_MODE_BOTH;
    cfg.i2s_iface = iface;
    int ret = es8374_init(&cfg);

    if (ret != 0)
    {
        printk("\r\nes8374 init fail\r\n");
    }
    else
    {
        printk("\r\nes8374 init ok!!!\r\n");
    }

    es8374_ctrl_state(cfg.es8374_mode, ES8374_CTRL_START);
    es8374_set_voice_volume(96);

    es8374_write_reg(0x1e, 0xA4);

    // es8374_read_all();

    i2s_io_mux_init();

    maix_i2s_init(ES8374_I2S_DEVICE, 0x3, 0x30);

    uint32_t i2s_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_I2S0);
    printk("i2s clock is: %d\r\n", i2s_freq);

    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0_M, 31); // 16384000 / (16000 * 256) = 4 ;
    i2s_tx_channel_config(ES8374_I2S_DEVICE, ES8374_TX_CHANNEL, RESOLUTION_16_BIT, SCLK_CYCLES_32, TRIGGER_LEVEL_4, STANDARD_MODE);
    i2s_rx_channel_config(ES8374_I2S_DEVICE, ES8374_RX_CHANNEL, RESOLUTION_16_BIT, SCLK_CYCLES_32, TRIGGER_LEVEL_4, STANDARD_MODE);
    i2s_set_sample_rate(ES8374_I2S_DEVICE, 16000);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
static uint8_t es8474_play(audio_t *audio, uint8_t *audio_buf, uint32_t audio_len)
{
    maix_i2s_play(ES8374_I2S_DEVICE, ES8374_TX_DMAC_CHANNEL, audio_buf, audio_len, 512, 16, 1);
    return 0;
}

uint8_t audio_es8374_init(audio_t *audio)
{
    audio->config = music_play_init;
    audio->play = es8474_play;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////