#ifndef __CORE1_H
#define __CORE1_H

#include <stdint.h>

#include "global_config.h"

#if CONFIG_ENABLE_OUTPUT_JPEG

#include "cQueue.h"

/* clang-format off */
typedef enum _net_task_type
{
    TASK_UPLOAD                 = (0),

    TASK_CAL_PIC_FEA            = (2),
    TASK_CAL_PIC_FEA_RES        = (3),

} net_task_type_t;

/* clang-format on */

typedef struct _net_task
{
    net_task_type_t task_type;
    void *task_data;
} net_task_t;

typedef struct _upload_face_pic
{
    uint8_t *jpeg_addr;
    uint32_t jpeg_len;

} upload_face_pic_t;

/* clang-format on */
extern volatile Queue_t q_core1;

void send_jpeg_to_core1(uint8_t *image);
int core1_function(void *ctx);

#endif /* CONFIG_ENABLE_OUTPUT_JPEG */
#endif /* __CORE1_H */
