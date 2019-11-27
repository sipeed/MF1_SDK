// #include "audio_mic.h"
// #include "i2s.h"
// #include "dmac.h"
// #include "fpioa.h"
// #include "gpiohs.h"
// #include "sysctl.h"
// #include "face_lib.h"

// //#define SWAP16(x) ((x&0x00ff) << 8 | (x&0xff00) >> 8)

// audio_mic_t audio_mic;
// uint32_t g_rx_dma_buf[MIC_FRAME_LENGTH * 4];
// uint32_t g_index;

// static void audio_mic_io_mux_init()
// {
//     // I2S IN MIC
//     fpioa_set_function(MIC_I2S_IN_D0, FUNC_I2S0_IN_D0);
//     fpioa_set_function(MIC_I2S_WS,    FUNC_I2S0_WS);
//     fpioa_set_function(MIC_I2S_SCLK,  FUNC_I2S0_SCLK);
// }

// static int audio_mic_dma_rx_irq(void *ctx)
// {
//     audio_mic_t *mic = (audio_mic_t *)ctx;
//     if(mic->recv_state != RECV_STATE_RECVING)
//     {
//         return 0;
//     }
//     static uint16_t recv_frame_count = 0;
//     uint32_t frame_index = recv_frame_count * mic->recv_frame_length;
//     if(g_index)
//     {
//         i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[g_index], mic->recv_frame_length * 2, mic->dmac_channel);
//         g_index = 0;
//         for(int i = 0; i < mic->recv_frame_length; i++)
//         {
//             mic->recv_buffer[i + frame_index] = (int16_t)((g_rx_dma_buf[2 * i + 1] & 0xffff));
//         }
//         recv_frame_count += 1;
//     }
//     else
//     {
//         i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[0], mic->recv_frame_length * 2, mic->dmac_channel);
//         g_index = mic->recv_buffer_length * 2;
//         for(int i = 0; i < mic->recv_frame_length; i++)
//         {
//             mic->recv_buffer[i + frame_index] = (int16_t)((g_rx_dma_buf[2 * i + g_index + 1] & 0xffff));
//         }
//         recv_frame_count += 1;
//     }
//     if(mic->recv_mode == RECV_MODE_ALWAYS)
//     {
//         if(recv_frame_count >= mic->recv_buffer_frames)
//         {
//             recv_frame_count = 0;
//             mic->recv_state = RECV_STATE_READY;
//         }
//     }else if(mic->recv_mode == RECV_MODE_TIMING)
//     {
//         if(recv_frame_count >= mic->recv_time_frames)
//         {
//             recv_frame_count = 0;
//             mic->recv_state = RECV_STATE_READY;
//         }
//     }else
//     {
//         recv_frame_count = 0;   
//     }
//     return 0;
// }

// void audio_mic_init()
// {
//     audio_mic_t *mic = &audio_mic;
//     sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
//     audio_mic_io_mux_init();

//     mic->i2s_device_num = MIC_I2S_DEVICE;
//     mic->i2s_channel_num = MIC_I2S_CHANNEL;
//     mic->dmac_channel = MIC_DMAC_CHANNEL;

//     mic->recv_frame_length = MIC_FRAME_LENGTH;
//     mic->sample_rate = MIC_SAMPLE_RATE;
//     mic->recv_mode = RECV_MODE_ALWAYS;
//     mic->recv_state = RECV_STATE_IDLE;
//     mic->recv_buffer = NULL;

//     i2s_init(mic->i2s_device_num, I2S_RECEIVER, 0x3);
//     i2s_rx_channel_config(mic->i2s_device_num, mic->i2s_channel_num, 
//         RESOLUTION_16_BIT, SCLK_CYCLES_32, 
//         TRIGGER_LEVEL_4, STANDARD_MODE);
//     i2s_set_sample_rate(mic->i2s_device_num, mic->sample_rate);

//     //dmac_init();
//     dmac_irq_register(mic->dmac_channel, audio_mic_dma_rx_irq, mic, 6);

// }
// void audio_mic_deinit()
// {
//     audio_mic_t *mic = &audio_mic;
//     dmac_irq_unregister(mic->dmac_channel);
// }

// void audio_mic_set_sample_rate(uint32_t sample_rate)
// {
//     audio_mic_t *mic = &audio_mic;
//     mic->sample_rate = sample_rate;
//     i2s_set_sample_rate(mic->i2s_device_num, mic->sample_rate);
// }
// void audio_mic_set_mode(audio_recv_mode_t mode)
// {
//     audio_mic_t *mic = &audio_mic;
//     mic->recv_mode = mode;
// }
// void audio_mic_set_timing_mode_time(uint32_t ms)
// {
//     audio_mic_t *mic = &audio_mic;
//     mic->recv_time_ms = ms;
//     mic->recv_time_frames = (uint32_t)((float)mic->sample_rate / 1000.f * mic->recv_time_ms) / mic->recv_frame_length;
//     if (mic->recv_time_frames > mic->recv_buffer_frames)
//     {
//         mic->recv_time_frames = mic->recv_buffer_frames;   // if you buffer shorter than need length, it will be set same as buffer length.
//     }
// }
// void audio_mic_set_buffer(int16_t *buf, uint64_t length)
// {
//     audio_mic_t *mic = &audio_mic;
//     mic->recv_buffer = buf;
//     mic->recv_buffer_length = length;
//     mic->recv_buffer_frames = mic->recv_buffer_length / mic->recv_frame_length;
// }
// void audio_mic_set_flash_address(uint32_t address, uint64_t length);

// uint32_t audio_mic_get_sample_rate()
// {
//     audio_mic_t *mic = &audio_mic;
//     return mic->sample_rate;
// }
// audio_recv_mode_t audio_mic_get_mode()
// {
//     audio_mic_t *mic = &audio_mic;
//     return mic->recv_mode;
// }
// audio_recv_state_t audio_mic_get_state()
// {
//     audio_mic_t *mic = &audio_mic;
//     return mic->recv_state;
// }
// uint32_t audio_mic_get_timing_mode_time()
// {
//     audio_mic_t *mic = &audio_mic;
//     return mic->recv_time_ms;
// }

// void audio_mic_start()
// {
//     audio_mic_t *mic = &audio_mic;
//     mic->recv_state = RECV_STATE_RECVING;
//     i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[0], mic->recv_frame_length * 2, mic->dmac_channel);
// }
// void audio_mic_stop()
// {
//     audio_mic_t *mic = &audio_mic;
//     mic->recv_state = RECV_STATE_IDLE;
// }
// void audio_mic_clear()
// {
//     audio_mic_t *mic = &audio_mic;
//     if (mic->recv_state == RECV_STATE_READY)
//     {
//         i2s_receive_data_dma(mic->i2s_device_num, &g_rx_dma_buf[0], mic->recv_frame_length * 2, mic->dmac_channel);
//         mic->recv_state = RECV_STATE_RECVING;
//     }
// }

// void audio_mic_save_to_flash(uint32_t addr, int16_t* save_buf, uint32_t length)
// {
//     w25qxx_write_data(addr, (uint8_t *)save_buf, sizeof(int16_t) * length);
// }