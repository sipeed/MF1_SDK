#ifndef __HTTP_SAVE_FILE_H
#define __HTTP_SAVE_FILE_H

uint32_t http_save_file(eth_tcp_client_t *client,
                        char *resp_header, uint32_t resp_header_len,
                        uint8_t *file, uint32_t file_len);
#endif