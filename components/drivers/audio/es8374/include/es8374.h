#ifndef __ES8374_H
#define __ES8374_H

#include "music_play.h"

#include "i2c.h"
#include "i2s.h"
#include "dmac.h"

#define ES8374_I2C_DEVICE I2C_DEVICE_0

#define ES8374_I2S_DEVICE I2S_DEVICE_0
#define ES8374_RX_CHANNEL I2S_CHANNEL_0
#define ES8374_TX_CHANNEL I2S_CHANNEL_2

#define ES8374_RX_DMAC_CHANNEL DMAC_CHANNEL3
#define ES8374_TX_DMAC_CHANNEL DMAC_CHANNEL4

/* ES8374 address */
#define ES8374_ADDR 0x10 // 0x22:CE=1;0x20:CE=0

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

    typedef enum
    {
        BIT_LENGTH_MIN = -1,
        BIT_LENGTH_16BITS = 0x03,
        BIT_LENGTH_18BITS = 0x02,
        BIT_LENGTH_20BITS = 0x01,
        BIT_LENGTH_24BITS = 0x00,
        BIT_LENGTH_32BITS = 0x04,
        BIT_LENGTH_MAX,
    } es_bits_length_t;

    typedef enum
    {
        MCLK_DIV_MIN = -1,
        MCLK_DIV_1 = 1,
        MCLK_DIV_2 = 2,
        MCLK_DIV_3 = 3,
        MCLK_DIV_4 = 4,
        MCLK_DIV_6 = 5,
        MCLK_DIV_8 = 6,
        MCLK_DIV_9 = 7,
        MCLK_DIV_11 = 8,
        MCLK_DIV_12 = 9,
        MCLK_DIV_16 = 10,
        MCLK_DIV_18 = 11,
        MCLK_DIV_22 = 12,
        MCLK_DIV_24 = 13,
        MCLK_DIV_33 = 14,
        MCLK_DIV_36 = 15,
        MCLK_DIV_44 = 16,
        MCLK_DIV_48 = 17,
        MCLK_DIV_66 = 18,
        MCLK_DIV_72 = 19,
        MCLK_DIV_5 = 20,
        MCLK_DIV_10 = 21,
        MCLK_DIV_15 = 22,
        MCLK_DIV_17 = 23,
        MCLK_DIV_20 = 24,
        MCLK_DIV_25 = 25,
        MCLK_DIV_30 = 26,
        MCLK_DIV_32 = 27,
        MCLK_DIV_34 = 28,
        MCLK_DIV_7 = 29,
        MCLK_DIV_13 = 30,
        MCLK_DIV_14 = 31,
        MCLK_DIV_MAX,
    } es_sclk_div_t;

    typedef enum
    {
        LCLK_DIV_MIN = -1,
        LCLK_DIV_128 = 0,
        LCLK_DIV_192 = 1,
        LCLK_DIV_256 = 2,
        LCLK_DIV_384 = 3,
        LCLK_DIV_512 = 4,
        LCLK_DIV_576 = 5,
        LCLK_DIV_768 = 6,
        LCLK_DIV_1024 = 7,
        LCLK_DIV_1152 = 8,
        LCLK_DIV_1408 = 9,
        LCLK_DIV_1536 = 10,
        LCLK_DIV_2112 = 11,
        LCLK_DIV_2304 = 12,

        LCLK_DIV_125 = 16,
        LCLK_DIV_136 = 17,
        LCLK_DIV_250 = 18,
        LCLK_DIV_272 = 19,
        LCLK_DIV_375 = 20,
        LCLK_DIV_500 = 21,
        LCLK_DIV_544 = 22,
        LCLK_DIV_750 = 23,
        LCLK_DIV_1000 = 24,
        LCLK_DIV_1088 = 25,
        LCLK_DIV_1496 = 26,
        LCLK_DIV_1500 = 27,
        LCLK_DIV_MAX,
    } es_lclk_div_t;

    typedef enum
    {
        D2SE_PGA_GAIN_MIN = -1,
        D2SE_PGA_GAIN_DIS = 0,
        D2SE_PGA_GAIN_EN = 1,
        D2SE_PGA_GAIN_MAX = 2,
    } es_d2se_pga_t;

    typedef enum
    {
        ADC_INPUT_MIN = -1,
        ADC_INPUT_LINPUT1_RINPUT1 = 0x00,
        ADC_INPUT_MIC1 = 0x05,
        ADC_INPUT_MIC2 = 0x06,
        ADC_INPUT_LINPUT2_RINPUT2 = 0x50,
        ADC_INPUT_DIFFERENCE = 0xf0,
        ADC_INPUT_MAX,
    } es_adc_input_t;

    typedef enum
    {
        DAC_OUTPUT_MIN = -1,
        DAC_OUTPUT_LOUT1 = 0x04,
        DAC_OUTPUT_LOUT2 = 0x08,
        DAC_OUTPUT_SPK = 0x09,
        DAC_OUTPUT_ROUT1 = 0x10,
        DAC_OUTPUT_ROUT2 = 0x20,
        DAC_OUTPUT_ALL = 0x3c,
        DAC_OUTPUT_MAX,
    } es_dac_output_t;

    typedef enum
    {
        MIC_GAIN_MIN = -1,
        MIC_GAIN_0DB = 0,
        MIC_GAIN_3DB = 3,
        MIC_GAIN_6DB = 6,
        MIC_GAIN_9DB = 9,
        MIC_GAIN_12DB = 12,
        MIC_GAIN_15DB = 15,
        MIC_GAIN_18DB = 18,
        MIC_GAIN_21DB = 21,
        MIC_GAIN_24DB = 24,
        MIC_GAIN_MAX,
    } es_mic_gain_t;

    typedef enum
    {
        ES_MODULE_MIN = -1,
        ES_MODULE_ADC = 0x01,
        ES_MODULE_DAC = 0x02,
        ES_MODULE_ADC_DAC = 0x03,
        ES_MODULE_LINE = 0x04,
        ES_MODULE_MAX
    } es_module_t;

    typedef enum
    {
        ES_MODE_MIN = -1,
        ES_MODE_SLAVE = 0x00,
        ES_MODE_MASTER = 0x01,
        ES_MODE_MAX,
    } es_mode_t;

    typedef enum
    {
        ES_I2S_MIN = -1,
        ES_I2S_NORMAL = 0,
        ES_I2S_LEFT = 1,
        ES_I2S_RIGHT = 2,
        ES_I2S_DSP = 3,
        ES_I2S_MAX
    } es_i2s_fmt_t;

    /**
 * @brief Configure ES8388 clock
 */
    typedef struct
    {
        es_sclk_div_t sclk_div; /*!< bits clock divide */
        es_lclk_div_t lclk_div; /*!< WS clock divide */
    } es_i2s_clock_t;

    typedef enum _es8374_mode_t
    {
        ES8374_MODE_ENCODE = 1,
        ES8374_MODE_DECODE,
        ES8374_MODE_BOTH,
        ES8374_MODE_LINE_IN,
    } es8374_mode_t;

    typedef enum _es8374_adc_input_t
    {
        ES8374_ADC_INPUT_LINE1 = 0x00,
        ES8374_ADC_INPUT_LINE2,
        ES8374_ADC_INPUT_ALL,
        ES8374_ADC_INPUT_DIFFERENCE,
    } es8374_adc_input_t;

    typedef enum _es8374_dac_output_t
    {
        ES8374_DAC_OUTPUT_LINE1 = 0x00,
        ES8374_DAC_OUTPUT_LINE2,
        ES8374_DAC_OUTPUT_ALL,
    } es8374_dac_output_t;

    typedef enum _es8374_ctrl_t
    {
        ES8374_CTRL_STOP = 0x00,
        ES8374_CTRL_START = 0x01,
    } es8374_ctrl_t;

    typedef enum _es8374_iface_mode_t
    {
        ES8374_MODE_SLAVE = 0x00,
        ES8374_MODE_MASTER = 0x01,
    } es8374_iface_mode_t;

    typedef enum _es8374_iface_samples_t
    {
        ES8374_08K_SAMPLES,
        ES8374_11K_SAMPLES,
        ES8374_16K_SAMPLES,
        ES8374_22K_SAMPLES,
        ES8374_24K_SAMPLES,
        ES8374_32K_SAMPLES,
        ES8374_44K_SAMPLES,
        ES8374_48K_SAMPLES,
    } es8374_iface_samples_t;

    typedef enum _es8374_iface_bits_t
    {
        ES8374_BIT_LENGTH_16BITS = 1,
        ES8374_BIT_LENGTH_24BITS,
        ES8374_BIT_LENGTH_32BITS,
    } es8374_iface_bits_t;

    typedef enum _es8374_iface_format_t
    {
        ES8374_I2S_NORMAL = 0,
        ES8374_I2S_LEFT,
        ES8374_I2S_RIGHT,
        ES8374_I2S_DSP,
    } es8374_iface_format_t;

    typedef struct _es8374_i2s_iface_t
    {
        es8374_iface_mode_t mode;
        es8374_iface_format_t fmt;
        es8374_iface_samples_t samples;
        es8374_iface_bits_t bits;
    } es8374_i2s_iface_t;

    typedef struct _es8374_config_t
    {
        es8374_adc_input_t adc_input;
        es8374_dac_output_t dac_output;
        es8374_mode_t es8374_mode;
        es8374_i2s_iface_t i2s_iface;
    } es8374_config_t;

    int maix_write_reg(uint8_t addr, uint8_t reg, uint8_t data);
    int maix_read_reg(uint8_t addr, uint8_t reg, uint8_t *data);
    //static
    int maix_i2c_init();

    int es8374_write_reg(uint8_t reg, uint8_t data);
    int es8374_read_reg(uint8_t reg, uint8_t *data);

    void es8374_read_all();

    int es8374_set_voice_mute(int enable);
    int es8374_get_voice_mute();

    int es8374_set_bits_per_sample(es_module_t mode, es_bits_length_t bit_per_sample);

    int es8374_config_fmt(es_module_t mode, es_i2s_fmt_t fmt);

    int es8374_start(es_module_t mode);
    int es8374_stop(es_module_t mode);

    int es8374_i2s_config_clock(es_i2s_clock_t cfg);
    int es8374_config_dac_output(es_dac_output_t output);
    int es8374_config_adc_input(es_adc_input_t input);
    int es8374_set_mic_gain(es_mic_gain_t gain);
    int es8374_set_voice_volume(int volume);
    int es8374_get_voice_volume(int *volume);

    int es8374_init(es8374_config_t *cfg);
    int es8374_deinit(void);

    int es8374_config_i2s(es8374_mode_t mode, es8374_i2s_iface_t *iface);
    int es8374_ctrl_state(es8374_mode_t mode, es8374_ctrl_t ctrl_state);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__ES83744_H