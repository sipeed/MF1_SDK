#ifndef __UART_RECV_H
#define __UART_RECV_H

#include "flash.h"
#include "face_lib.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern volatile uint8_t recv_over_flag;
extern volatile uint8_t jpeg_recv_start_flag;

extern uint32_t jpeg_recv_len;
extern uint64_t jpeg_recv_start_time;

extern uint8_t cJSON_prase_buf[PROTOCOL_BUF_LEN];
extern uint8_t jpeg_recv_buf[JPEG_BUF_LEN];

extern pkt_head_t g_pkt_head;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t protocol_init_device(board_cfg_t *brd_cfg, uint8_t op);

void protocol_start_recv_jpeg(void);
void protocol_stop_recv_jpeg(void);

void init_relay_key_pin(board_cfg_t *brd_cfg);
void init_lcd_cam(board_cfg_t *brd_cfg);

#endif
