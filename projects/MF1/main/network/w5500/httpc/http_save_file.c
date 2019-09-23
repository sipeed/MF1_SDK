#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "sleep.h"
#include "sysctl.h"

#include "EthernetClient.h"

#include "http_save_file.h"

/* clang-format off */
#define BUFFSIZE                2048
/* clang-format on */

static uint32_t l_recv_len = 0;

static uint32_t l_recv_file_buf_len = 0;
static uint8_t *l_recv_file_buf = NULL;

static uint32_t l_recv_header_buf_len = 0;
static uint8_t *l_recv_header_buf = NULL;

static uint8_t l_tmp_buf[BUFFSIZE + 1] = {0};

/*read buffer by byte still delim ,return read bytes counts*/
static uint32_t read_until(char *buffer, char delim, uint32_t len)
{
    /*TODO: delim check,buffer check,further: do an buffer length limited*/
    uint32_t i = 0;
    while (buffer[i] != delim && i < len)
    {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return 1 if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return 0
 * */
static uint32_t read_past_header(char text[], uint32_t total_len)
{
    /* i means current position */
    uint32_t i = 0, i_read_len = 0;

    while (text[i] != 0 && i < total_len)
    {
        i_read_len = read_until(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2)
        {
            uint32_t i_write_len = total_len - (i + 2);

            l_recv_len = 0;
            memset(l_recv_file_buf, 0, l_recv_file_buf_len);
            memcpy(l_recv_file_buf + l_recv_len, &(text[i + 2]), i_write_len);
            l_recv_len += i_write_len;

            memset(l_recv_header_buf, 0, l_recv_header_buf_len);
            memcpy(l_recv_header_buf, text, (i > l_recv_header_buf_len) ? l_recv_header_buf_len : i);
            l_recv_header_buf[l_recv_header_buf_len] = 0;

            return 1;
        }
        i += i_read_len;
    }
    return 0;
}

uint32_t http_save_file(eth_tcp_client_t *client,
                        char *resp_header, uint32_t resp_header_len,
                        uint8_t *file, uint32_t file_len)
{
    //check file buf
    l_recv_file_buf = file;
    l_recv_file_buf_len = file_len;
    l_recv_header_buf = (uint8_t *)resp_header;
    l_recv_header_buf_len = resp_header_len;

    if ((!l_recv_file_buf) || (l_recv_file_buf_len == 0) || (!l_recv_header_buf) || (l_recv_header_buf_len == 0))
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    uint64_t start = sysctl_get_time_us();
    // printf("start:%ld\r\n", start);

    //receive
    int per_packet_len = 0, buff_len = 0;
    int resp_body_start = 0, flag = 1, poll_times;

    /*deal with all receive packet*/
    while (flag)
    {
        memset(l_tmp_buf, 0, BUFFSIZE);

        poll_times = 500; //超时500ms,FIXME: 这里是否需要等这么久。
        do
        {
            buff_len = client->available(client);
            if (buff_len > 0)
                break;

            uint8_t stat = client->status(client);
            // printk("%02X ", stat);
            if (stat == SnSR_CLOSE_WAIT ||
                stat == SnSR_CLOSED)
            {
                buff_len = 0;
                break;
            }
            // printk("$");
            usleep(500);
        } while (--poll_times > 0);

        // printf("len:%d\r\n", buff_len);

        if (buff_len == 0)
        {
            /* receive error or no data*/
            flag = 0;
            printf("%s\r\n", l_recv_len ? "receive over" : "receive failed");
        }
        else if (buff_len > 0 && !resp_body_start)
        {
            per_packet_len = (buff_len > BUFFSIZE) ? BUFFSIZE : buff_len;
            client->read(client, l_tmp_buf, per_packet_len);
            /*deal with response header*/
            resp_body_start = read_past_header(l_tmp_buf, per_packet_len);
            //这里取出header中的内容长度
        }
        else if (buff_len > 0 && resp_body_start)
        {
            per_packet_len = (buff_len > BUFFSIZE) ? BUFFSIZE : buff_len;
            client->read(client, l_tmp_buf, per_packet_len);
            memcpy(l_recv_file_buf + l_recv_len, l_tmp_buf, per_packet_len);
            l_recv_len += per_packet_len;
        }

        if (l_recv_len > l_recv_file_buf_len)
        {
            printf("l_recv_len > l_recv_file_buf_len\r\n");
            flag = 0;
        }
    }

    client->stop(client);

    //destory client
    client->destory(client);

    uint64_t end = sysctl_get_time_us();
    printf("recv resopnse use time: %ld us\r\n", end - start);

    return l_recv_len;
}