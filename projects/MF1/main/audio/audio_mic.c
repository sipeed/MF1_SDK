#include "audio_mic.h"
#include "i2s.h"
#include "dmac.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "sysctl.h"
#include "face_lib.h"
#include "es8374.h"

//#define SWAP16(x) ((x&0x00ff) << 8 | (x&0xff00) >> 8)
#if USE_ES8374
extern volatile uint8_t g_es8374_init_flag;
#endif

audio_mic_t audio_mic;
uint32_t g_rx_dma_buf[MIC_FRAME_LENGTH * 4];
uint32_t g_index;

static void audio_mic_io_mux_init()
{

#if USE_ES8374
    fpioa_set_function(MIC_I2S_IN,    FUNC_I2S0_IN_D0);
    fpioa_set_function(MIC_I2S_WS,    FUNC_I2S0_WS);
    fpioa_set_function(MIC_I2S_SCLK,  FUNC_I2S0_SCLK);
    fpioa_set_function(MIC_I2S_MCLK,  FUNC_I2S0_MCLK);
#else
    // I2S IN MIC
    fpioa_set_function(MIC_I2S_IN_D0, FUNC_I2S0_IN_D0);
    fpioa_set_function(MIC_I2S_WS,    FUNC_I2S0_WS);
    fpioa_set_function(MIC_I2S_SCLK,  FUNC_I2S0_SCLK);

#endif

}

static int audio_mic_dma_rx_irq(void *ctx)
{
    audio_mic_t *mic = (audio_mic_t *)ctx;
    if(mic->recv_state != RECV_STATE_RECVING)
    {
        return 0;
    }
    static uint16_t recv_frame_count = 0;
    uint32_t frame_index = recv_frame_count * mic->recv_frame_length;
    if(g_index)
    {
        i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[g_index], mic->recv_frame_length * 2, mic->dmac_channel);
        g_index = 0;
        for(int i = 0; i < mic->recv_frame_length; i++)
        {
            mic->recv_buffer[i + frame_index] = (int16_t)((g_rx_dma_buf[2 * i + 1] & 0xffff));
        }
        recv_frame_count += 1;
    }
    else
    {
        i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[0], mic->recv_frame_length * 2, mic->dmac_channel);
        g_index = mic->recv_buffer_length * 2;
        for(int i = 0; i < mic->recv_frame_length; i++)
        {
            mic->recv_buffer[i + frame_index] = (int16_t)((g_rx_dma_buf[2 * i + g_index + 1] & 0xffff));
        }
        recv_frame_count += 1;
    }
    if(mic->recv_mode == RECV_MODE_ALWAYS)
    {
        if(recv_frame_count >= mic->recv_buffer_frames)
        {
            recv_frame_count = 0;
            mic->recv_state = RECV_STATE_READY;
        }
    }else if(mic->recv_mode == RECV_MODE_TIMING)
    {
        if(recv_frame_count >= mic->recv_time_frames)
        {
            recv_frame_count = 0;
            mic->recv_state = RECV_STATE_READY;
        }
    }else
    {
        recv_frame_count = 0;   
    }
    return 0;
}

void audio_mic_init()
{
    audio_mic_t *mic = &audio_mic;
    mic->i2s_device_num = MIC_I2S_DEVICE;
    mic->i2s_channel_num = MIC_I2S_CHANNEL;
    mic->dmac_channel = MIC_DMAC_CHANNEL;

    mic->recv_frame_length = MIC_FRAME_LENGTH;
    mic->sample_rate = MIC_SAMPLE_RATE;
    mic->recv_mode = RECV_MODE_ALWAYS;
    mic->recv_state = RECV_STATE_IDLE;
    mic->recv_buffer = NULL;

#if USE_ES8374
    audio_mic_io_mux_init();

    if(!g_es8374_init_flag)
    {
        sysctl_pll_set_freq(SYSCTL_PLL2, 262144000UL);
        int ret;
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
        ret = es8374_init(&cfg);
        
        if(ret != 0)
        {
            printf("\r\nes8374 init fail\r\n");
        }else
        {
            printf("\r\nes8374 init ok!!!\r\n");
        }
        es8374_ctrl_state(cfg.es8374_mode, ES8374_CTRL_START);

        maix_i2s_init(MIC_I2S_DEVICE, 0x3, 0x30);
        sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0_M, 31);
        i2s_tx_channel_config(MIC_I2S_DEVICE, ES8374_TX_CHANNEL, RESOLUTION_16_BIT, SCLK_CYCLES_32, TRIGGER_LEVEL_4, STANDARD_MODE);
        i2s_rx_channel_config(MIC_I2S_DEVICE, ES8374_RX_CHANNEL, RESOLUTION_16_BIT, SCLK_CYCLES_32, TRIGGER_LEVEL_4, STANDARD_MODE);
        i2s_set_sample_rate(MIC_I2S_DEVICE, MIC_SAMPLE_RATE);
        es8374_set_voice_volume(96);
        g_es8374_init_flag = 1;
    }

#else
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
    audio_mic_io_mux_init();

    i2s_init(mic->i2s_device_num, I2S_RECEIVER, 0x3);
    i2s_rx_channel_config(mic->i2s_device_num, mic->i2s_channel_num, 
        RESOLUTION_16_BIT, SCLK_CYCLES_32, 
        TRIGGER_LEVEL_4, STANDARD_MODE);
    i2s_set_sample_rate(mic->i2s_device_num, mic->sample_rate);
#endif
    //dmac_init();
    dmac_irq_register(mic->dmac_channel, audio_mic_dma_rx_irq, mic, 6);

}
void audio_mic_deinit()
{
    audio_mic_t *mic = &audio_mic;
    dmac_irq_unregister(mic->dmac_channel);
#if USE_ES8374
    es8374_deinit();
    g_es8374_init_flag = 0;
#endif
}

void audio_mic_set_sample_rate(uint32_t sample_rate)
{
    audio_mic_t *mic = &audio_mic;
    mic->sample_rate = sample_rate;
    i2s_set_sample_rate(mic->i2s_device_num, mic->sample_rate);
}
void audio_mic_set_mode(audio_recv_mode_t mode)
{
    audio_mic_t *mic = &audio_mic;
    mic->recv_mode = mode;
}
void audio_mic_set_timing_mode_time(uint32_t ms)
{
    audio_mic_t *mic = &audio_mic;
    mic->recv_time_ms = ms;
    mic->recv_time_frames = (uint32_t)((float)mic->sample_rate / 1000.f * mic->recv_time_ms) / mic->recv_frame_length;
    if (mic->recv_time_frames > mic->recv_buffer_frames)
    {
        mic->recv_time_frames = mic->recv_buffer_frames;   // if you buffer shorter than need length, it will be set same as buffer length.
    }
}
void audio_mic_set_buffer(int16_t *buf, uint64_t length)
{
    audio_mic_t *mic = &audio_mic;
    mic->recv_buffer = buf;
    mic->recv_buffer_length = length;
    mic->recv_buffer_frames = mic->recv_buffer_length / mic->recv_frame_length;
}
void audio_mic_set_flash_address(uint32_t address, uint64_t length);

uint32_t audio_mic_get_sample_rate()
{
    audio_mic_t *mic = &audio_mic;
    return mic->sample_rate;
}
audio_recv_mode_t audio_mic_get_mode()
{
    audio_mic_t *mic = &audio_mic;
    return mic->recv_mode;
}
audio_recv_state_t audio_mic_get_state()
{
    audio_mic_t *mic = &audio_mic;
    return mic->recv_state;
}
uint32_t audio_mic_get_timing_mode_time()
{
    audio_mic_t *mic = &audio_mic;
    return mic->recv_time_ms;
}

void audio_mic_start()
{
    audio_mic_t *mic = &audio_mic;
    mic->recv_state = RECV_STATE_RECVING;
    i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[0], mic->recv_frame_length * 2, mic->dmac_channel);
}
void audio_mic_stop()
{
    audio_mic_t *mic = &audio_mic;
    mic->recv_state = RECV_STATE_IDLE;
}
void audio_mic_clear()
{
    audio_mic_t *mic = &audio_mic;
    if (mic->recv_state == RECV_STATE_READY)
    {
        i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[0], mic->recv_frame_length * 2, mic->dmac_channel);
        mic->recv_state = RECV_STATE_RECVING;
    }
}

void audio_mic_save_to_flash(uint32_t addr, int16_t* save_buf, uint32_t length)
{
    w25qxx_write_data(addr, (uint8_t *)save_buf, sizeof(int16_t) * length);
}