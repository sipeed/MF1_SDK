
#include "core1.h"

#if CONFIG_ENABLE_OUTPUT_JPEG

#include <bsp.h>
#include "atomic.h"
#include "sysctl.h"
#include "uart.h"

#include "jpeg_encode.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define JPEG_BUF_LEN (20 * 1024)
static uint8_t jpeg_buf[JPEG_BUF_LEN];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// volatile Queue_t q_core0; // Queue declaration
volatile Queue_t q_core1; // Queue declaration

// volatile uint32_t core0_queue_flag = 0;

// static volatile spinlock_t q_core0_lock;
static volatile spinlock_t q_core1_lock;

// void queue_core0_lock(void)
// {
//     spinlock_lock(&q_core0_lock);
// }

// void queue_core0_unlock(void)
// {
//     spinlock_unlock(&q_core0_lock);
// }

static void queue_core1_lock(void)
{
    spinlock_lock(&q_core1_lock);
}

static void queue_core1_unlock(void)
{
    spinlock_unlock(&q_core1_lock);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void send_jpeg_to_core1(uint8_t *image)
{
    jpeg_encode_t jpeg_src, jpeg_out;
    uint64_t v_tim;

    {
        v_tim = sysctl_get_time_us();

        jpeg_src.w = CONFIG_CAMERA_RESOLUTION_WIDTH;
        jpeg_src.h = CONFIG_CAMERA_RESOLUTION_HEIGHT;
        jpeg_src.bpp = 2;
        jpeg_src.data = image;

        jpeg_out.w = jpeg_src.w;
        jpeg_out.h = jpeg_src.h;
        jpeg_out.bpp = JPEG_BUF_LEN;
        jpeg_out.data = jpeg_buf;

        v_tim = sysctl_get_time_us();
        if (jpeg_compress(&jpeg_src, &jpeg_out, 80, 0) == 0)
        {
            printk("jpeg encode use %ld us\r\n", sysctl_get_time_us() - v_tim);
            printk("w:%d\th:%d\tbpp:%d\r\n", jpeg_out.w, jpeg_out.h, jpeg_out.bpp);
        }
        else
        {
            printk("jpeg encode failed\r\n");
            return;
        }

        printk("q_core1 size:%d\r\n", q_core1.cnt);

        if (q_getCount(&q_core1) >= 10)
        {
            printk("too many face wait to upload drop this one\r\n");
            return;
        }

        net_task_t *task = NULL;
        bool queue_stat = false;
        upload_face_pic_t *upload = NULL;

        task = (net_task_t *)malloc(sizeof(net_task_t));
        if (task)
        {
            task->task_type = TASK_UPLOAD;
        }
        else
        {
            printk("%d malloc failed\r\n", __LINE__);
            return;
        }

        upload = (upload_face_pic_t *)malloc(sizeof(upload_face_pic_t));
        if (upload)
        {
            upload->jpeg_len = jpeg_out.bpp;
            upload->jpeg_addr = (uint8_t *)malloc(sizeof(uint8_t) * upload->jpeg_len);
            if (upload->jpeg_addr)
            {
                memcpy(upload->jpeg_addr, jpeg_out.data, upload->jpeg_len);

                task->task_data = (void *)upload;

                queue_core1_lock();
                queue_stat = q_push(&q_core1, &task);
                queue_core1_unlock();

                if (!queue_stat)
                {
                    printk("%d====== Queue push failed!\r\n", __LINE__);
                    free(upload->jpeg_addr);
                    free(upload);
                    free(task);
                }
            }
            else
            {
                printk("%d malloc failed\r\n", __LINE__);
                free(upload);
                free(task);
            }
        }
        else
        {
            printk("%d malloc failed\r\n", __LINE__);
            free(task);
            return;
        }
    }
}

int core1_function(void *ctx)
{
    printk("Core %ld Hello world\n", current_coreid());

    uint8_t have_pic = 0;

    bool queue_stat = false;
    net_task_t *task = NULL;

    while (1)
    {
        queue_core1_lock();
        if (!q_isEmpty(&q_core1))
        {
            queue_stat = q_pop(&q_core1, &task);
            if (queue_stat && task)
            {
                have_pic = 1;
            }
        }
        queue_core1_unlock();

        if (have_pic)
        {
            have_pic = 0;
            if (task)
            {
                switch (task->task_type)
                {
                case TASK_UPLOAD:
                {
                    printk("upload  face\r\n");

                    upload_face_pic_t *upload = (upload_face_pic_t *)task->task_data;

                    uint32_t total, sent = 0, per_send = 0;

                    total = upload->jpeg_len;

                    while (sent < total)
                    {
                        //FIXME: 优化这个
                        uart_send_data(UART_DEV2, upload->jpeg_addr + sent, 1);
                        sent++;
                    }

                    free(upload->jpeg_addr);
                    free(upload);
                }
                break;
                default:
                {
                    printk("core1 unknown task_type %d\r\n", task->task_type);
                }
                break;
                }
                free(task);
                task = NULL;
            }
            else
            {
                printk("%d====== Queue pop failed!\r\n", __LINE__);
            }
        }
    }
}
#endif /* CONFIG_ENABLE_OUTPUT_JPEG */