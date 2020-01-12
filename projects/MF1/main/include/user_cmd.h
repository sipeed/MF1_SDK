#ifndef __USER_CMD_H
#define __USER_CMD_H

#include "global_config.h"

#include <stdint.h>

#include "face_lib.h"

///////////////////////////////////////
#if CONFIG_NOTIFY_STRANGER
extern protocol_custom_cb_t user_custom_cmd[4];
#else
extern protocol_custom_cb_t user_custom_cmd[3];
#endif
///////////////////////////////////////
extern int proto_scan_qrcode_flag;
extern uint8_t proto_start_face_recon_flag;

void proto_qrcode_scan_loop(void);

///////////////////////////////////////

#endif /* __USER_CMD_H */
