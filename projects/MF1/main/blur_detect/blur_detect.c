#include "blur_detect.h"
#include "stdlib.h"
#include "math.h"

float conv_data[9 * 3 * 3] = {
//R
    0,1,0,1,-4,1,0,1,0,
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
//G
    0,0,0,0,0,0,0,0,0,
    0,1,0,1,-4,1,0,1,0,
    0,0,0,0,0,0,0,0,0,
//B
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,
    0,1,0,1,-4,1,0,1,0,
};

volatile uint8_t g_conv_done_flag;

conv_img_t conv_out_img;

//plic_irq_callback_t
static int kpu_done(void *ctx)
{
	g_conv_done_flag = 1;
	return 0;
}

static kpu_task_t blur_detect_task;

void blur_detect_init(uint16_t w, uint16_t h, uint8_t ch)
{
    layer_conv_init(&blur_detect_task, w, h, ch, ch, conv_data);
    conv_out_img.w = w;
    conv_out_img.h = h;
    conv_out_img.ch = ch;
    conv_out_img.buf = malloc(sizeof(uint8_t) * ch * w * h);
}

float blur_detect_run(uint8_t *buf_in)
{
    float result = 0;
    layer_conv_run(&blur_detect_task, buf_in, conv_out_img.buf, kpu_done);
    while (!g_conv_done_flag);
    g_conv_done_flag = 0;
    result = clc_variance(&conv_out_img);
    return result;
}

static float clc_variance(conv_img_t *conv_img)
{
    uint8_t ch_mean[conv_img->ch];
    uint64_t ch_sum[conv_img->ch];
    uint64_t ch_variance[conv_img->ch];
    float result = 0;
    for (int i = 0; i < conv_img->ch; i++)
    {
        ch_mean[i] = 0;
        ch_variance[i] = 0;
    }
    uint32_t img_size = conv_img->w * conv_img->h;

    for (int i = 0; i < conv_img->ch; i++)
    {
        for (uint32_t j = 0; j < img_size; j++)
        {
            ch_sum[i] += conv_img->buf[j * conv_img->ch + i];
        }
        ch_mean[i] = ch_sum[i]/img_size;
    }

    for (int i = 0; i < conv_img->ch; i++)
    {
        for (uint32_t j = 0; j < img_size; j++)
        {
            ch_variance[i] += (ch_mean[i] - (conv_img->buf[j * conv_img->ch + i])) * (ch_mean[i] - (conv_img->buf[j * conv_img->ch + i]));
        }
        ch_variance[i] /= img_size;
    }
    for (int i = 0; i < conv_img->ch; i++)
    {
        result += ch_variance[i] / ch_mean[i];
    }
    result /= conv_img->ch;

    return result;
}

void blur_detect_deinit()
{
    
    free(conv_out_img.buf);
}