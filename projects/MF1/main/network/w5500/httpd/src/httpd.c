/* mini_httpd - small HTTP server
**
** Copyright � 1999,2000 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include "httpd.h"
#include "httpd_mime.h"

#include "match.h"
#include "tdate_parse.h"

#include "web_config.h"
#include "webpage.h"

///////////////////////////////////////////////////////////////////////////////
static void do_file(httpd_req_t *req);

static int auth_check(httpd_req_t *req, char *dirname);

static char *get_mime_type(char *name);
static char *get_method_str(int m);
static char *get_request_line(httpd_req_t *req);

static web_cmd_t *get_cmd_handle(char *query_cmd, uint8_t method);
static web_html_t *get_file_handle(char *filename);

static int add_to_post_data(httpd_post_data_t **post_data, char *str, int len);
static void deal_multipart(httpd_req_t *req);

static void add_to_request(httpd_req_t *req, char *str, size_t len);
static void add_headers(httpd_req_t *req, int s, char *title, char *extra_header, char *me, char *mt, long b, time_t mod);
static void add_to_response(httpd_req_t *req, char *str);

static void send_error(httpd_req_t *req, int s, char *title, char *extra_header, char *text);
static void send_response(httpd_req_t *req);

///////////////////////////////////////////////////////////////////////////////
int httpd_read(httpd_req_t *req, char *buf, int len)
{
    eth_tcp_client_t *client = req->client;

#if 0
	int poll_times = 2 * 100; //100ms

	int read = 0, buff_len = 0;
	int real_read_len = 0, per_read_len = 0;

	while (read < len)
	{
		do
		{
			buff_len = client->available(client);
			if (buff_len > 0)
			{
				printf("####%s:%d\r\n", __func__, __LINE__);
				break;
			}

			uint8_t stat = client->status(client);
			printf("stat:%d\r\n", stat);
			if (stat == SnSR_CLOSE_WAIT ||
				stat == SnSR_CLOSED)
			{
				buff_len = 0;
				printf("####%s:%d\r\n", __func__, __LINE__);
				break;
			}
			usleep(500);
		} while (--poll_times > 0);

		if (buff_len == 0)
		{
			printf("####%s:%d\r\n", __func__, __LINE__);
			return read;
		}
		else
		{
			per_read_len = (buff_len > (len - read)) ? (len - read) : buff_len;
			real_read_len = client->read(client, buf + read, per_read_len);
		}
		read += real_read_len;
	}
	printf("####%s:%d\r\n", __func__, __LINE__);

	return read;
#else
    int read = 0, buff_len = 0;
    buff_len = client->available(client);
    if (buff_len > 0)
    {
        read = client->read(client, buf + read, (buff_len > len) ? len : buff_len);
    }
    return read;
#endif
}

int httpd_write(httpd_req_t *req, char *buf, int len)
{
    if (len <= 0)
        return 0;

    eth_tcp_client_t *client = req->client;

    len = client->write_fix(client, buf, len, 100);
    client->flush(client);

    return len;
}

///////////////////////////////////////////////////////////////////////////////
static uint8_t httpd_req_init(httpd_req_t *req)
{
    req->remoteuser = NULL;
    req->method = METHOD_UNKNOWN;
    req->path = NULL;
    req->file = NULL;
    req->pathinfo = NULL;
    req->query = NULL;
    req->protocol = NULL;
    req->status = 0;
    req->bytes = -1;
    req->req_hostname = NULL;
    req->authorization = NULL;
    req->content_type = NULL;
    req->content_length = -1;
    req->cookie = NULL;
    req->host = NULL;
    req->if_modified_since = (time_t)-1;
    req->referrer = NULL;
    req->useragent = NULL;

    req->response = NULL;
    req->response_size = 0;
    req->response_len = 0;

    req->post_data = NULL;
    return 0;
}

static void finish_request(httpd_req_t *req)
{
    if (req->request)
        free(req->request);

    if (req->response)
        free(req->response);

    if (req->post_data)
    {
        if (req->post_data->buf)
            free(req->post_data->buf);
        free(req->post_data);
    }
}

void httpd_handle_request(eth_tcp_client_t *client)
{
    char buf[2048];
    char *method_str;
    char *line;
    char *cp;
    int r;

    ///////////////////////////////////////////////////////////////////////////////
    httpd_req_t httpd_req, *req;
    req = &httpd_req;
    req->client = client;

    ///////////////////////////////////////////////////////////////////////////////
    /* Initialize the request variables. */
    httpd_req_init(req);

    /* Read in the request. */
    req->request_size = 0;
    req->request_idx = 0;

    //read out all header
    for (;;)
    {
        int rr = httpd_read(req, buf, sizeof(buf) - 1);
        if (rr <= 0)
            break;
        add_to_request(req, buf, rr);
        if (strstr(req->request, "\r\n\r\n") != NULL ||
            strstr(req->request, "\n\n") != NULL)
            break;
    }

    /* Parse the first line of the request. */
    method_str = get_request_line(req);
    if (method_str == NULL)
        send_error(req, 400, "Bad Request", "", "Can't parse request.");
    req->path = strpbrk(method_str, " \t\n\r");
    if (req->path == NULL)
        send_error(req, 400, "Bad Request", "", "Can't parse request.");
    *(req->path)++ = '\0';
    req->path += strspn(req->path, " \t\n\r");
    req->protocol = strpbrk(req->path, " \t\n\r");
    if (req->protocol == NULL)
        send_error(req, 400, "Bad Request", "", "Can't parse request.");
    *(req->protocol)++ = '\0';
    req->protocol += strspn(req->protocol, " \t\n\r");
    req->query = strchr(req->path, '?');
    if (req->query == NULL)
        req->query = "";
    else
        *(req->query)++ = '\0';

    /* Parse the rest of the request headers. */
    while ((line = get_request_line(req)) != NULL)
    {
        if (line[0] == '\0')
            break;
        else if (strncasecmp(line, "Authorization:", 14) == 0)
        {
            cp = &line[14];
            cp += strspn(cp, " \t");
            req->authorization = cp;
        }
        else if (strncasecmp(line, "Content-Length:", 15) == 0)
        {
            cp = &line[15];
            cp += strspn(cp, " \t");
            req->content_length = atol(cp);
        }
        else if (strncasecmp(line, "Content-Type:", 13) == 0)
        {
            cp = &line[13];
            cp += strspn(cp, " \t");
            req->content_type = cp;
        }
        else if (strncasecmp(line, "Cookie:", 7) == 0)
        {
            cp = &line[7];
            cp += strspn(cp, " \t");
            req->cookie = cp;
        }
        else if (strncasecmp(line, "Host:", 5) == 0)
        {
            cp = &line[5];
            cp += strspn(cp, " \t");
            req->host = cp;
            if (req->host[0] == '\0' || req->host[0] == '.' ||
                strchr(req->host, '/') != NULL)
                send_error(req, 400, "Bad Request", "", "Can't parse request.");
        }
        else if (strncasecmp(line, "If-Modified-Since:", 18) == 0)
        {
            cp = &line[18];
            cp += strspn(cp, " \t");
            req->if_modified_since = tdate_parse(cp);
        }
        else if (strncasecmp(line, "Referer:", 8) == 0)
        {
            cp = &line[8];
            cp += strspn(cp, " \t");
            req->referrer = cp;
        }
        else if (strncasecmp(line, "Referrer:", 9) == 0)
        {
            cp = &line[9];
            cp += strspn(cp, " \t");
            req->referrer = cp;
        }
        else if (strncasecmp(line, "User-Agent:", 11) == 0)
        {
            cp = &line[11];
            cp += strspn(cp, " \t");
            req->useragent = cp;
        }
    }

    if (strcasecmp(method_str, get_method_str(METHOD_GET)) == 0)
        req->method = METHOD_GET;
    else if (strcasecmp(method_str, get_method_str(METHOD_HEAD)) == 0)
        req->method = METHOD_HEAD;
    else if (strcasecmp(method_str, get_method_str(METHOD_POST)) == 0)
        req->method = METHOD_POST;
    else if (strcasecmp(method_str, get_method_str(METHOD_PUT)) == 0)
        req->method = METHOD_PUT;
    else if (strcasecmp(method_str, get_method_str(METHOD_DELETE)) == 0)
        req->method = METHOD_DELETE;
    else if (strcasecmp(method_str, get_method_str(METHOD_TRACE)) == 0)
        req->method = METHOD_TRACE;
    else
        send_error(req, 501, "Not Implemented", "", "That method is not implemented.");

    strdecode(req->path, req->path);
    if (req->path[0] != '/')
    {
        send_error(req, 400, "Bad Request", "", "Bad filename.");
    }
    req->file = &(req->path[1]);
    de_dotdot(req->file);

    if (req->file[0] == '\0')
    {
        //default index
        req->file = config_default_page;
    }

    if (req->file[0] == '/' ||
        (req->file[0] == '.' && req->file[1] == '.' &&
         (req->file[2] == '\0' || req->file[2] == '/')))
    {
        send_error(req, 400, "Bad Request", "", "Illegal filename.");
        goto _end;
    }

    /*read post body */
    if (req->method == METHOD_POST)
    {
        int remain;
        int total_len;

        total_len = req->content_length;
        remain = req->request_len - req->request_idx;

        printf("req->content_type:%s\r\n", req->content_type);

        if (remain > 0 && (add_to_post_data(&(req->post_data), &(req->request[req->request_idx]), remain) < 0))
        {
            printf("error at %s:%d\r\n", __func__, __LINE__);
            goto _end;
        }
        if (total_len > remain)
        {
            r = httpd_read(req, buf, MIN(sizeof(buf), total_len - remain));
            if (r <= 0)
            {
                printf("error at %s:%d\r\n", __func__, __LINE__);
                goto _end;
            }
            if (add_to_post_data(&(req->post_data), buf, r) < 0)
            {
                printf("error at %s:%d\r\n", __func__, __LINE__);
                goto _end;
            }
            if (req->request == NULL)
                goto _end;
            remain = req->request_len - req->request_idx + r;
        }

        while (remain < total_len)
        {
            r = httpd_read(req, buf, MIN(sizeof(buf), total_len - remain));
            if (r <= 0)
            {
                printf("error at %s:%d\r\n", __func__, __LINE__);
                goto _end;
            }
            {
                if (add_to_post_data(&(req->post_data), buf, r) < 0)
                    goto _end;
            }
            remain += r;
        }

        if (total_len != req->content_length)
        {
            printf("####%s:%d\r\n", __func__, __LINE__);
            while (remain < req->content_length)
            {
                r = httpd_read(req, buf, MIN(sizeof(buf), req->content_length - remain));
                if (r <= 0)
                    break;
                //here is needed?
                {
                    if (add_to_post_data(&(req->post_data), buf, r) < 0)
                        goto _end;
                }
                remain += r;
            }
        }
    }

    //deal request
    do_file(req);
_end:
    finish_request(req);

    client->stop(client);
    client->destory(client);
    return;
}

static void do_file(httpd_req_t *req)
{
    int err_code = HTTP_REQUEST_OK;
    int last_mod = time_pages_created;

    const char *mime_type;
    char *cp;

    char buf[1024];
    char fixed_mime_type[250];
    char header[255];

    /* Check authorization for this directory. */
#if ENABLE_AUTH
    (void)strncpy(buf, req->file, sizeof(buf));
    cp = strrchr(buf, '/');
    if (cp == (char *)0)
        (void)strcpy(buf, ".");
    else
        *cp = '\0';
    printf("http auth path:%s\r\n", buf);
    printf("authorization:%s\r\n", req->authorization);

    /* 这个验证没有掉线机制 */
    /* 用户登录一次,就永久不需要登录, 客户有需求再加吧.*/
    if (auth_check(req, buf) < 0)
    {
        printf("auth failed\r\n");
        return;
    }
#endif

    if (req->method != METHOD_GET && req->method != METHOD_POST)
    {
        send_error(req, 501, "Not Implemented", "", "That method is not implemented.");
        return;
    }

    mime_type = get_mime_type(req->file);
    (void)snprintf(fixed_mime_type, sizeof(fixed_mime_type), mime_type, CHAERSET);

    if (req->if_modified_since != (time_t)-1)
    {
        /* we don't hope browser get pages from cache */
        last_mod = (req->if_modified_since < time_pages_created) ? time_pages_created : req->if_modified_since + 1;
    }

    {
        switch (req->method)
        {
        case METHOD_GET:
        {
            web_cmd_t *cmd = get_cmd_handle(req->file, METHOD_GET);
            if (cmd)
            {
                printf("user query cmd: %s\r\n", req->file);
                cmd->func(req);
                return;
            }
            else
            {
                web_html_t *file = get_file_handle(req->file);

                if (file)
                {
                    printf("user query file: %s\r\n", req->file);

                    add_headers(req, 200, "Ok", NULL, NULL, NULL, -1, last_mod);
                    send_response(req);

                    httpd_write(req, file->addr, file->size);
                    return;
                }
            }
            err_code = HTTP_NOT_FOUND;
            goto err_out;
        }
        break;
        case METHOD_POST:
        {
            web_cmd_t *cmd = get_cmd_handle(req->file, METHOD_POST);
            if (cmd)
            {
                printf("user query cmd: %s\r\n", req->file);
                cmd->func(req);
                return;
            }
            err_code = HTTP_NOT_FOUND;
            goto err_out;
        }
        break;
        }
    }
err_out:
    switch (err_code)
    {
    case HTTP_UNAUTHORIZED:
        snprintf(header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", "/");
        send_error(req, HTTP_UNAUTHORIZED, "Unauthorized", header, "Authorization required.");
        break;

    case HTTP_FORBIDDEN:
        send_error(req, HTTP_FORBIDDEN, "Forbidden", (char *)0, "Forbidden.");
        break;

    case HTTP_BAD_REQUEST:
        send_error(req, HTTP_BAD_REQUEST, "Bad Request", (char *)0, "Bad request.");
        break;

    case HTTP_NOT_FOUND:
        send_error(req, HTTP_NOT_FOUND, "Not Found", (char *)0, "File not found.");
        break;

    case HTTP_METHOD_NA:
        send_error(req, HTTP_METHOD_NA, "Not Support", (char *)0, "Method not supported.");
        break;

    case HTTP_ERROR:
        send_error(req, HTTP_ERROR, "Server Error", (char *)0, "Internal server error.");
        break;

    default:
        break;
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static web_cmd_t *get_cmd_handle(char *query_cmd, uint8_t method)
{
    web_cmd_t *entry;
    char *p;

    entry = &web_cmd_table[0];

    while (entry->cmd_path)
    {
        p = entry->cmd_path;
        while (*p == '/' || *p == '\\')
            p++;

        if (strcmp(query_cmd, p) == 0)
        {
            if (method == entry->method)
            {
                break;
            }
        }

        entry++;
    }

    return (entry->cmd_path) ? entry : NULL;
}

static web_html_t *get_file_handle(char *filename)
{
    web_html_t *entry;
    char *p;

    entry = &web_htmlfile_table[0];
    while (entry->url_path)
    {
        p = entry->url_path;
        while (*p == '/' || *p == '\\')
            p++;
        if (strcmp(filename, p) == 0)
            break;
        entry++;
    }

    if (entry->url_path)
    {
        return entry;
    }

    printf("not found file:%s\r\n", filename);

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void add_to_response(httpd_req_t *req, char *str)
{
    add_str(&(req->response), &(req->response_size), &(req->response_len), str);
}

static void send_response(httpd_req_t *req)
{
    (void)httpd_write(req, (req->response), (req->response_len));
}

static void add_to_request(httpd_req_t *req, char *str, size_t len)
{
    add_data(&(req->request), &(req->request_size), &(req->request_len), str, len);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static char *get_method_str(int m)
{
    switch (m)
    {
    case METHOD_GET:
        return "GET";
    case METHOD_HEAD:
        return "HEAD";
    case METHOD_POST:
        return "POST";
    case METHOD_PUT:
        return "PUT";
    case METHOD_DELETE:
        return "DELETE";
    case METHOD_TRACE:
        return "TRACE";
    default:
        return "UNKNOWN";
    }
}

static char *get_request_line(httpd_req_t *req)
{
    int i;
    char c;

    for (i = req->request_idx; req->request_idx < req->request_len; ++req->request_idx)
    {
        c = req->request[req->request_idx];
        if (c == '\n' || c == '\r')
        {
            req->request[req->request_idx] = '\0';
            ++req->request_idx;
            if (c == '\r' && req->request_idx < req->request_len &&
                req->request[req->request_idx] == '\n')
            {
                req->request[req->request_idx] = '\0';
                ++(req->request_idx);
            }
            return &(req->request[i]);
        }
    }
    return (char *)0;
}

static char *get_mime_type(char *name)
{
    int fl = strlen(name);
    int i;

    for (i = 0; i < sizeof(mime_table) / sizeof(*mime_table); ++i)
    {
        int el = strlen(mime_table[i].ext);
        if (strcasecmp(&(name[fl - el]), mime_table[i].ext) == 0)
            return mime_table[i].type;
    }
    return "text/plain; charset=%s";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void add_headers(httpd_req_t *req, int s, char *title, char *extra_header, char *me, char *mt, long b, time_t mod)
{
    time_t now, expires;
    char timebuf[100];
    char buf[1024];
    int s100;
    const char *rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";

    req->status = s;
    req->bytes = b;
    req->response_len = 0; // start_response();
    (void)snprintf(buf, sizeof(buf), "%s %d %s\r\n", req->protocol, req->status, title);
    add_to_response(req, buf);
    (void)snprintf(buf, sizeof(buf), "Server: %s\r\n", SERVER_SOFTWARE);
    add_to_response(req, buf);
    //FIXME:
    now = time((time_t *)0);
    (void)strftime(timebuf, sizeof(timebuf), rfc1123_fmt, gmtime(&now));
    (void)snprintf(buf, sizeof(buf), "Date: %s\r\n", timebuf);
    add_to_response(req, buf);
    s100 = req->status / 100;
    if (s100 != 2 && s100 != 3)
    {
        (void)snprintf(buf, sizeof(buf), "Cache-Control: no-cache,no-store\r\n");
        add_to_response(req, buf);
    }
    if (extra_header != (char *)0 && extra_header[0] != '\0')
    {
        (void)snprintf(buf, sizeof(buf), "%s\r\n", extra_header);
        add_to_response(req, buf);
    }

    if (me != (char *)0 && me[0] != '\0')
    {
        (void)snprintf(buf, sizeof(buf), "Content-Encoding: %s\r\n", me);
        add_to_response(req, buf);
    }

    if (mt != (char *)0 && mt[0] != '\0')
    {
        (void)snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", mt);
        add_to_response(req, buf);
    }
    if (req->bytes >= 0)
    {
        (void)snprintf(
            buf, sizeof(buf), "Content-Length: %lld\r\n", (long long)req->bytes);
        add_to_response(req, buf);
    }
    // if (p3p != (char *)0 && p3p[0] != '\0')
    // {
    //     (void)snprintf(buf, sizeof(buf), "P3P: %s\r\n", p3p);
    //     add_to_response(req, buf);
    // }
    // if (max_age >= 0)
    // {
    //     expires = now + max_age;
    //     (void)strftime(
    //         timebuf, sizeof(timebuf), rfc1123_fmt, gmtime(&expires));
    //     (void)snprintf(buf, sizeof(buf),
    //                    "Cache-Control: max-age=%d\r\nExpires: %s\r\n", max_age, timebuf);
    //     add_to_response(req, buf);
    // }
    if (mod != (time_t)-1)
    {
        (void)strftime(
            timebuf, sizeof(timebuf), rfc1123_fmt, gmtime(&mod));
        (void)snprintf(buf, sizeof(buf), "Last-Modified: %s\r\n", timebuf);
        add_to_response(req, buf);
    }
    (void)snprintf(buf, sizeof(buf), "Connection: close\r\n\r\n");
    add_to_response(req, buf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int add_to_post_data(httpd_post_data_t **post_data, char *str, int len)
{
    char *old;
    int reserve_len = 255; //预留来为realloc的空间,具体值需要验证一下

    if (*post_data == 0)
    {
        *post_data = calloc(1, sizeof(httpd_post_data_t));
        if (*post_data == 0)
        {
            printf("Memory full @ %s:%d\r\n", __func__, __LINE__);
            return -1;
        }
        (*post_data)->size = len + reserve_len;
        (*post_data)->buf = malloc((*post_data)->size);
        (*post_data)->len = 0;
    }
    else if ((*post_data)->len + len >= (*post_data)->size)
    {
        (*post_data)->size = (*post_data)->len + len + reserve_len;
        old = (*post_data)->buf;
        (*post_data)->buf = realloc((*post_data)->buf, (*post_data)->size);
        if ((*post_data)->buf == NULL)
        {
            free(old);
            printf("Memory full @ %s:%d\r\n", __func__, __LINE__);
            return -1;
        }
    }

    if ((*post_data)->buf == NULL)
    {
        printf("Memory full @ %s:%d\r\n", __func__, __LINE__);
        free(*post_data);
        *post_data = 0;
        return -1;
    }
    memcpy(&((*post_data)->buf[(*post_data)->len]), str, len);
    (*post_data)->len += len;
    (*post_data)->buf[(*post_data)->len] = '\0';

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int send_error_file(httpd_req_t *req, int err_code)
{
    char *filename = NULL;
    web_html_t *fileptr;
    err_html_t *err_html;

    for (err_html = &err_pages[0]; err_html->fname; err_html++)
    {
        if (err_html->err_code == err_code)
        {
            filename = err_html->fname;
            break;
        }
    }

    if (!filename || !(fileptr = (web_html_t *)get_file_handle(filename)))
        return 0;

    send_response(req); /* send head first */

    httpd_write(req, (uint8_t *)fileptr->addr, fileptr->size);
    return 1;
}

static void send_error_body(httpd_req_t *req, int s, char *title, char *text)
{
    char buf[1024];

    if (send_error_file(req, s))
        return;

    /* Send built-in error page. */
    (void)snprintf(
        buf, sizeof(buf), "<html>\n\n<head>\n<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n\
        <title>%d %s</title>\n</head>\n\n<body bgcolor=\"#cc9999\" text=\"#000000\" link=\"#2020ff\" vlink=\"#4040cc\">\n\
        \n<h4>%d %s</h4>\n",
        s, title, s, title);
    add_to_response(req, buf);
    (void)snprintf(buf, sizeof(buf), "%s\n", text);
    add_to_response(req, buf);
}

static void send_error_tail(httpd_req_t *req)
{
    char buf[500];

    if (match("**MSIE**", req->useragent))
    {
        int n;
        (void)snprintf(buf, sizeof(buf), "<!--\n");
        add_to_response(req, buf);
        for (n = 0; n < 6; ++n)
        {
            (void)snprintf(buf, sizeof(buf), "Padding so that MSIE deigns to show this error instead of its own canned one.\n");
            add_to_response(req, buf);
        }
        (void)snprintf(buf, sizeof(buf), "-->\n");
        add_to_response(req, buf);
    }

    (void)snprintf(buf, sizeof(buf), "<hr>\n\n<address><a href=\"%s\">%s</a></address>\n\n</body>\n\n</html>\n", SERVER_URL, SERVER_SOFTWARE);
    add_to_response(req, buf);
}

static void send_error(httpd_req_t *req, int s, char *title, char *extra_header, char *text)
{
    add_headers(req, s, title, extra_header, "", "text/html; charset=%s", (long)-1, (time_t)-1);
    send_error_body(req, s, title, text);
    send_error_tail(req);
    send_response(req);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if ENABLE_AUTH
static void send_authenticate(httpd_req_t *req, char *realm)
{
    char header[128];

    (void)snprintf(header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", realm);
    send_error(req, 401, "Unauthorized", header, "Authorization required.");
}

static int auth_check(httpd_req_t *req, char *dirname)
{
    int l;
    char authinfo[128];
    char *authpass;
    char *colon;

    /* Does this request contain authorization info? */
    if (req->authorization == (char *)0)
    {
        /* Nope, return a 401 Unauthorized. */
        send_authenticate(req, dirname);
        return -1;
    }

    /* Basic authorization info? */
    if (strncmp(req->authorization, "Basic ", 6) != 0)
    {
        send_authenticate(req, dirname);
        return -1;
    }

    /* Decode it. */
    l = b64_decode(&(req->authorization[6]), (unsigned char *)authinfo, sizeof(authinfo) - 1);
    authinfo[l] = '\0';
    printf("authinfo:%s\r\n", authinfo);
    /* Split into user and password. */
    authpass = strchr(authinfo, ':');
    if (authpass == (char *)0)
    {
        /* No colon?  Bogus auth info. */
        send_authenticate(req, dirname);
        return -1;
    }
    *authpass++ = '\0';
    printf("authuser:%s\r\n", authinfo);
    printf("authpass:%s\r\n", authpass);
    /* If there are more fields, cut them off. */
    colon = strchr(authpass, ':');
    if (colon != (char *)0)
        *colon = '\0';
    printf("colon:%s\r\n", colon);

    /* check user name */
    if (strcmp(authinfo, AUTH_USER) != 0)
    {
        send_authenticate(req, dirname);
        return -1;
    }

    /* check user passwd */
    if (strcmp(authpass, AUTH_PASSWD) != 0)
    {
        send_authenticate(req, dirname);
        return -1;
    }

    return 0;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
