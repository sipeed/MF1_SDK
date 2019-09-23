#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "sleep.h"
#include "sysctl.h"

#include "EthernetClient.h"

#include "parsed_url.h"
#include "http_file.h"
#include "http_save_file.h"

/*
    Send http get request to download file
    return : len ok, receive file length
             0 failed
*/
uint32_t http_get_file(char *url,
                       char *custom_headers,
                       char *resp_header,
                       uint32_t resp_header_len,
                       uint8_t *file,
                       uint32_t file_len)
{
    if ((!resp_header) || (resp_header_len == 0) || (!file) || (file_len == 0))
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    struct parsed_url *url_parsed = parse_url(url);
    if (url_parsed == NULL)
    {
        printf("parse_url error\r\n");
        return 0;
    }

#if 0
    printf("url_parsed->uri:\t%s\r\n", url_parsed->uri);
    printf("url_parsed->scheme:\t%s\r\n", url_parsed->scheme);
    printf("url_parsed->host:\t%s\r\n", url_parsed->host);
    printf("url_parsed->ip:\t%s\r\n", url_parsed->ip);
    printf("url_parsed->port:\t%s\r\n", url_parsed->port);
    printf("url_parsed->path:\t%s\r\n", url_parsed->path);
    printf("url_parsed->query:\t%s\r\n", url_parsed->query);

#if NOT_ONLY_SUPPORT_GET
    printf("url_parsed->fragment:\t%s\r\n", url_parsed->fragment);
    printf("url_parsed->username:\t%s\r\n", url_parsed->username);
    printf("url_parsed->password:\t%s\r\n", url_parsed->password);
#endif
#endif
    /* Declare variable */
    char *headers = (char *)malloc(1024);
    char *headers_tmp = (char *)malloc(1024);

    if (!headers || !headers_tmp)
    {
        printf("error @ %d\r\n", __LINE__);
        if (headers)
            free(headers);
        if (headers_tmp)
            free(headers_tmp);
        parsed_url_free(url_parsed);
        return 0;
    }

    /* Build query/headers */
    if (url_parsed->path != NULL)
    {
        if (url_parsed->query != NULL)
        {
            sprintf(headers_tmp, "GET /%s?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", url_parsed->path, url_parsed->query, url_parsed->host);
        }
        else
        {
            sprintf(headers_tmp, "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", url_parsed->path, url_parsed->host);
        }
    }
    else
    {
        if (url_parsed->query != NULL)
        {
            sprintf(headers_tmp, "GET /?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", url_parsed->query, url_parsed->host);
        }
        else
        {
            sprintf(headers_tmp, "GET / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", url_parsed->host);
        }
    }

#if NOT_ONLY_SUPPORT_GET
    // /* Handle authorisation if needed */
    // if (url_parsed->username != NULL)
    // {
    //     /* Format username:password pair */
    //     char *upwd = (char *)malloc(1024);
    //     sprintf(upwd, "%s:%s", url_parsed->username, url_parsed->password);
    //     upwd = (char *)realloc(upwd, strlen(upwd) + 1);

    //     /* Base64 encode */
    //     char *base64 = base64_encode(upwd);

    //     /* Form header */
    //     char *auth_header = (char *)malloc(1024);
    //     sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
    //     auth_header = (char *)realloc(auth_header, strlen(auth_header) + 1);

    //     /* Add to header */
    //     l_headers = (char *)realloc(l_headers, strlen(l_headers) + strlen(auth_header) + 2);
    //     sprintf(l_headers, "%s%s", l_headers, auth_header);
    // }
#endif

    /* Add custom headers, and close */
    if (custom_headers != NULL)
    {
        sprintf(headers, "%s%s\r\n", headers_tmp, custom_headers);
    }
    else
    {
        sprintf(headers, "%s\r\n", headers_tmp);
    }

    free(headers_tmp);
    headers = (char *)realloc(headers, strlen(headers) + 1);

    //start receive
    eth_tcp_client_t *client = NULL;
    if ((client = eth_tcp_client_new(MAX_SOCK_NUM, TCP_CLIENT_DEFAULT_TIMEOUT)) != NULL)
    {
        if (client->connect_domain(client, url_parsed->host, atoi(url_parsed->port)) == 1)
        {
            printf("connect to server ok\r\n");
        }
        else
        {
            printf("connect to server failed\r\n");
            free(headers);
            parsed_url_free(url_parsed);
            return 0;
        }
    }
    else
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    parsed_url_free(url_parsed);

    /* Send headers to server */
    int header_len = strlen(headers);
    if (client->write_fix(client, (uint8_t *)headers, header_len, 100) != header_len)
    {
        printf("Send header to server failed\n");
        free(headers);

        client->stop(client);
        //destory client
        client->destory(client);
        return 0;
    }
    free(headers);

    return http_save_file(client, resp_header, resp_header_len, file, file_len);
}

/*
    Post 文件到服务器
    header 需要添加 Content-Type: multipart/form-data; boundary=------WebKitFormBoundaryO2aA3WiAfUqIcD6e（可变,比正文少两个）

    Post文件，一般需要提交表单，所以需要添加表单字段，需要手动生成，传入的body，并且使用的boundary需要一并传入
    格式如下

    --boundary
    Content-Disposition: form-data; name="url"

    http://coolaf.com/tool/params?g=gtest&g2=gtest2
    boundary
    Content-Disposition: form-data; name="seltype"

    --boundary\r\n
    文件
    --boundary--\r\n

    ###这个仅供参考，具体实现时还请使用WireShark抓包分析！！！
*/

uint32_t http_post_file(char *url,
                        char *custom_headers,
                        uint8_t *body,
                        uint8_t *boundary,
                        uint8_t *post_file,
                        uint32_t post_file_len,
                        char *resp_header,
                        uint32_t resp_header_len,
                        uint8_t *file,
                        uint32_t file_len)
{
    if ((!post_file) || (post_file_len == 0))
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    if (!custom_headers)
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    if ((!resp_header) || (resp_header_len == 0) || (!file) || (file_len == 0))
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    struct parsed_url *url_parsed = parse_url(url);
    if (url_parsed == NULL)
    {
        printf("parse_url error\r\n");
        return 0;
    }

#if 0
    printf("url_parsed->uri:\t%s\r\n", url_parsed->uri);
    printf("url_parsed->scheme:\t%s\r\n", url_parsed->scheme);
    printf("url_parsed->host:\t%s\r\n", url_parsed->host);
    printf("url_parsed->ip:\t%s\r\n", url_parsed->ip);
    printf("url_parsed->port:\t%s\r\n", url_parsed->port);
    printf("url_parsed->path:\t%s\r\n", url_parsed->path);
    printf("url_parsed->query:\t%s\r\n", url_parsed->query);
#if NOT_ONLY_SUPPORT_GET
    printf("url_parsed->fragment:\t%s\r\n", url_parsed->fragment);
    printf("url_parsed->username:\t%s\r\n", url_parsed->username);
    printf("url_parsed->password:\t%s\r\n", url_parsed->password);
#endif
#endif

    /* Declare variable */
    char *headers = (char *)malloc(1024);
    char *headers_tmp = (char *)malloc(1024);

    if (!headers || !headers_tmp)
    {
        printf("error @ %d\r\n", __LINE__);
        if (headers)
            free(headers);
        if (headers_tmp)
            free(headers_tmp);
        parsed_url_free(url_parsed);
        return 0;
    }

    uint32_t content_len = post_file_len;

    if (body)
    {
        content_len += strlen((char *)body);
    }

    if (boundary)
    {
        content_len += strlen((char *)boundary) + 8;
    }

    /* Build query/headers */
    if (url_parsed->path != NULL)
    {
        if (url_parsed->query != NULL)
        {
            sprintf(headers_tmp, "POST /%s?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %d\r\n", url_parsed->path, url_parsed->query, url_parsed->host, content_len);
        }
        else
        {
            sprintf(headers_tmp, "POST /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %d\r\n", url_parsed->path, url_parsed->host, content_len);
        }
    }
    else
    {
        if (url_parsed->query != NULL)
        {
            sprintf(headers_tmp, "POST /?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %d\r\n", url_parsed->query, url_parsed->host, content_len);
        }
        else
        {
            sprintf(headers_tmp, "POST / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %d\r\n", url_parsed->host, content_len);
        }
    }

#if NOT_ONLY_SUPPORT_GET
    /* Handle authorisation if needed */
    // if (url_parsed->username != NULL)
    // {
    //     /* Format username:password pair */
    //     char *upwd = (char *)malloc(1024);
    //     sprintf(upwd, "%s:%s", url_parsed->username, url_parsed->password);
    //     upwd = (char *)realloc(upwd, strlen(upwd) + 1);

    //     /* Base64 encode */
    //     char *base64 = base64_encode(upwd);

    //     /* Form header */
    //     char *auth_header = (char *)malloc(1024);
    //     sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
    //     auth_header = (char *)realloc(auth_header, strlen(auth_header) + 1);

    //     /* Add to header */
    //     http_headers = (char *)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
    //     sprintf(http_headers, "%s%s", http_headers, auth_header);
    // }
#endif

    if (body)
        sprintf(headers, "%s%s\r\n%s", headers_tmp, custom_headers, body);
    else
        sprintf(headers, "%s%s\r\n", headers_tmp, custom_headers);

    free(headers_tmp);
    headers = (char *)realloc(headers, strlen(headers) + 1);

    // printf("headers:\r\n%s", headers);

    uint64_t start = sysctl_get_time_us();

    eth_tcp_client_t *client = NULL;
    if ((client = eth_tcp_client_new(MAX_SOCK_NUM, TCP_CLIENT_DEFAULT_TIMEOUT)) != NULL)
    {
        if (client->connect_domain(client, url_parsed->host, atoi(url_parsed->port)) == 1)
        {
            printf("connect to server ok\r\n");
        }
        else
        {
            printf("connect to server failed\r\n");
            free(headers);
            parsed_url_free(url_parsed);
            return 0;
        }
    }
    else
    {
        printf("error @ %d\r\n", __LINE__);
        return 0;
    }

    parsed_url_free(url_parsed);

    /* Send headers to server */
    int header_len = strlen(headers);

    if (client->write_fix(client, (uint8_t *)headers, header_len, 100) != header_len)
    {
        printf("Send header to server failed\n");
        free(headers);
        client->stop(client);
        //destory client
        client->destory(client);
        return 0;
    }
    free(headers);

    if (client->write_fix(client, post_file, post_file_len, 500) != post_file_len)
    {
        printf("Send post_file request to server failed\r\n");
        client->stop(client);
        //destory client
        client->destory(client);
        return 0;
    }

    if (boundary)
    {
        uint8_t *endboundary = (uint8_t *)malloc(sizeof(uint8_t) * 128);

        int end_len = sprintf((char *)endboundary, "\r\n--%s--\r\n", boundary);

        if (client->write_fix(client, endboundary, end_len, 100) != end_len)
        {
            printf("Send endboundary to server failed\r\n");
            client->stop(client);
            //destory client
            client->destory(client);
            return 0;
        }
        free(endboundary);
    }

    uint64_t end = sysctl_get_time_us();
    printf("post file use time: %ld us\r\n", end - start);

    return http_save_file(client, resp_header, resp_header_len, file, file_len);
}
