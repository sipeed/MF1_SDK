#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "i2s.h"
#include "stdlib.h"
#include "sysctl.h"
#include "utils.h"
#include "maix_i2s.h"


static void maix_i2s_set_enable(i2s_device_number_t device_num, uint32_t enable)
{
    ier_t u_ier;

    u_ier.reg_data = readl(&i2s[device_num]->ier);
    u_ier.ier.ien = enable;
    writel(u_ier.reg_data, &i2s[device_num]->ier);
}

static void maix_i2s_disable_block(i2s_device_number_t device_num, i2s_transmit_t rxtx_mode)
{
    irer_t u_irer;
    iter_t u_iter;

    if(rxtx_mode == I2S_RECEIVER)
    {
        u_irer.reg_data = readl(&i2s[device_num]->irer);
        u_irer.irer.rxen = 0;
        writel(u_irer.reg_data, &i2s[device_num]->irer);
        /* Receiver block disable */
    } else
    {
        u_iter.reg_data = readl(&i2s[device_num]->iter);
        u_iter.iter.txen = 0;
        writel(u_iter.reg_data, &i2s[device_num]->iter);
        /* Transmitter block disable */
    }
}

static int maix_i2s_set_mask_interrupt(i2s_device_number_t device_num,
                                  i2s_channel_num_t channel_num,
                                  uint32_t rx_available_int, uint32_t rx_overrun_int,
                                  uint32_t tx_empty_int, uint32_t tx_overrun_int)
{
    imr_t u_imr;

    if(channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;
    u_imr.reg_data = readl(&i2s[device_num]->channel[channel_num].imr);

    if(rx_available_int == 1)
        u_imr.imr.rxdam = 1;
    else
        u_imr.imr.rxdam = 0;
    if(rx_overrun_int == 1)
        u_imr.imr.rxfom = 1;
    else
        u_imr.imr.rxfom = 0;

    if(tx_empty_int == 1)
        u_imr.imr.txfem = 1;
    else
        u_imr.imr.txfem = 0;
    if(tx_overrun_int == 1)
        u_imr.imr.txfom = 1;
    else
        u_imr.imr.txfom = 0;
    writel(u_imr.reg_data, &i2s[device_num]->channel[channel_num].imr);
    return 0;
}

static int maix_i2s_transmit_channel_enable(i2s_device_number_t device_num,
                                       i2s_channel_num_t channel_num, uint32_t enable)
{
    ter_t u_ter;

    if(channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    u_ter.reg_data = readl(&i2s[device_num]->channel[channel_num].ter);
    u_ter.ter.txchenx = enable;
    writel(u_ter.reg_data, &i2s[device_num]->channel[channel_num].ter);
    return 0;
}

static void maix_i2s_transimit_enable(i2s_device_number_t device_num, i2s_channel_num_t channel_num)
{
    iter_t u_iter;

    u_iter.reg_data = readl(&i2s[device_num]->iter);
    u_iter.iter.txen = 1;
    writel(u_iter.reg_data, &i2s[device_num]->iter);
    /* Transmitter block enable */

    maix_i2s_transmit_channel_enable(device_num, channel_num, 1);
    /* Transmit channel enable */
}

static int maix_i2s_transmit_dma_enable(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;

    if(device_num >= I2S_DEVICE_MAX)
        return -1;

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.dma_tx_en = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);

    return 0;
}

static int maix_i2s_recv_channel_enable(i2s_device_number_t device_num,
                                   i2s_channel_num_t channel_num, uint32_t enable)
{
    rer_t u_rer;

    if(channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;
    u_rer.reg_data = readl(&i2s[device_num]->channel[channel_num].rer);
    u_rer.rer.rxchenx = enable;
    writel(u_rer.reg_data, &i2s[device_num]->channel[channel_num].rer);
    return 0;
}

static void maix_i2s_receive_enable(i2s_device_number_t device_num, i2s_channel_num_t channel_num)
{
    irer_t u_irer;

    u_irer.reg_data = readl(&i2s[device_num]->irer);
    u_irer.irer.rxen = 1;
    writel(u_irer.reg_data, &i2s[device_num]->irer);
    /* Receiver block enable */

    maix_i2s_recv_channel_enable(device_num, channel_num, 1);
    /* Receive channel enable */
}

static inline void maix_i2s_set_sign_expand_en(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;
    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.sign_expand_en = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);
}

static int maix_i2s_receive_dma_enable(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;

    if(device_num >= I2S_DEVICE_MAX)
        return -1;

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.dma_rx_en = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);

    return 0;
}

void maix_i2s_init(i2s_device_number_t device_num, uint32_t rx_channel_mask, uint32_t tx_channel_mask)
{
    sysctl_clock_enable(SYSCTL_CLOCK_I2S0 + device_num);
    sysctl_reset(SYSCTL_RESET_I2S0 + device_num);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0 + device_num, 7);

    maix_i2s_set_enable(device_num, 1);
    maix_i2s_disable_block(device_num, I2S_TRANSMITTER);
    maix_i2s_disable_block(device_num, I2S_RECEIVER);

    if( (tx_channel_mask & rx_channel_mask) != 0)
    {
        printf("Cannot use the same transceiver channel!\n");
        return;
    }

    if( tx_channel_mask != 0){
        for(int i = 0; i < 4; i++)
        {
            if((tx_channel_mask & 0x3) == 0x3)
            {
                maix_i2s_set_mask_interrupt(device_num, I2S_CHANNEL_0 + i, 1, 1, 1, 1);
                maix_i2s_transimit_enable(device_num, I2S_CHANNEL_0 + i);
            } else
            {
                maix_i2s_transmit_channel_enable(device_num, I2S_CHANNEL_0 + i, 0);
            }
            tx_channel_mask >>= 2;
        }
        maix_i2s_transmit_dma_enable(device_num, 1);
    }

    if (rx_channel_mask != 0)
    {
        for(int i = 0; i < 4; i++)
        {
            if((rx_channel_mask & 0x3) == 0x3)
            {
                maix_i2s_set_mask_interrupt(device_num, I2S_CHANNEL_0 + i, 1, 1, 1, 1);
                maix_i2s_receive_enable(device_num, I2S_CHANNEL_0 + i);
            } else
            {
                maix_i2s_recv_channel_enable(device_num, I2S_CHANNEL_0 + i, 0);
            }
            rx_channel_mask >>= 2;
        }
        /* Set expand_en when receive */
        maix_i2s_set_sign_expand_en(device_num, 1);
        maix_i2s_receive_dma_enable(device_num, 1);
    }
}