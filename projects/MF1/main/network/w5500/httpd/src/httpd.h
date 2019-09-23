#ifndef __HTTPD_H
#define __HTTPD_H

#include <stdlib.h>

#include "EthernetClient.h"

///////////////////////////////////////////////////////////////////////////////
/* clang-format off */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define SERVER_SOFTWARE             "mini_httpd/1.30 26Oct2018"
#define SERVER_URL                  "http://www.acme.com/software/mini_httpd/"
#define CHAERSET                    "utf-8"

#define ENABLE_AUTH                 (1)
#define AUTH_USER                   "admin"
#define AUTH_PASSWD                 "123456"

#define METHOD_UNKNOWN              (0)
#define METHOD_GET                  (1)
#define METHOD_HEAD                 (2)
#define METHOD_POST                 (3)
#define METHOD_PUT                  (4)
#define METHOD_DELETE               (5)
#define METHOD_TRACE                (6)

#define HTTP_REQUEST_OK				(200)
#define HTTP_BAD_REQUEST			(400)
#define HTTP_UNAUTHORIZED			(401)
#define HTTP_FORBIDDEN				(403)
#define HTTP_NOT_FOUND				(404)
#define HTTP_METHOD_NA				(405)
#define HTTP_ERROR					(500)
#define HTTP_NOT_IMPLEMENT			(501)

/* clang-format on */
///////////////////////////////////////////////////////////////////////////////
typedef struct _httpd_post_data httpd_post_data_t;
typedef struct _httpd_post_data
{
    int size; //should be more big than len
              //otherwise will realloc failed
    int len;
    char *buf;
    struct httpd_post_data_t *next;
} httpd_post_data_t;

typedef struct _httpd_req
{
    /* Request variables. */
    eth_tcp_client_t *client;

    char *request;
    size_t request_size, request_len, request_idx;
    int method;
    char *path;
    char *file;
    char *pathinfo;
    char *query;
    char *protocol;
    int status;
    long bytes;
    char *req_hostname;
    char *authorization;
    size_t content_length;
    char *content_type;
    char *cookie;
    char *host;
    time_t if_modified_since;
    char *referrer;
    char *useragent;
    char *remoteuser;

    char *response;
    size_t response_size, response_len;

    httpd_post_data_t *post_data;

} httpd_req_t;

void httpd_handle_request(eth_tcp_client_t *client);

int httpd_read(httpd_req_t *req, char *buf, int len);
int httpd_write(httpd_req_t *req, char *buf, int len);

///////////////////////////////////////////////////////////////////////////////
/* httpd_util.c */
void add_data(char **bufP, size_t *bufsizeP, size_t *buflenP, char *str, size_t len);
void add_str(char **bufP, size_t *bufsizeP, size_t *buflenP, char *str);

void strdecode(char *to, char *from);
void de_dotdot(char *f);

int b64_decode(const char *str, unsigned char *space, int size);

///////////////////////////////////////////////////////////////////////////////

#endif
