#include "webpage.h"

#include "sha256.h"

#include "lcd.h"
#include "cJSON.h"

///////////////////////////////////////////////////////////////////////////////
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#include "incbin.h"

INCBIN(WEB_INDEX, "html/index.html");
///////////////////////////////////////////////////////////////////////////////
/* 可以将网页资源打包成一个bin文件,在头部做一个table,读取tabl来解析,节省内存资源 */
web_html_t web_htmlfile_table[] = {
    {"index.html", WEB_INDEX_data, 0, 0},
    {NULL, NULL, 0, 0},
};

time_t time_pages_created = __TIMESTAMP__;

///////////////////////////////////////////////////////////////////////////////
static uint8_t http_get_led_stat(httpd_req_t *req);
static uint8_t http_set_led_stat(httpd_req_t *req);

web_cmd_t web_cmd_table[] = {
    {"get_led_stat", http_get_led_stat, METHOD_GET},
    {"set_led_stat", http_set_led_stat, METHOD_POST},
    {NULL, NULL},
};

///////////////////////////////////////////////////////////////////////////////
void webpage_init_size(void)
{
    web_htmlfile_table[0].size = WEB_INDEX_size;
}

///////////////////////////////////////////////////////////////////////////////
static uint8_t led_stat[3] = {0, 0, 0};
static char *stat_json = "{\"r\":%d,\"g\":%d,\"b\":%d}";

static uint8_t http_get_led_stat(httpd_req_t *req)
{
    char buf[128];
    size_t len;

    printf("####%s:%d\r\n", __func__, __LINE__);

    len = sprintf(buf, stat_json, led_stat[0], led_stat[1], led_stat[2]);
    httpd_write(req, buf, len);
}

static cJSON *cJSON_GetObjectItem_Type(const cJSON *const object, const char *const string, uint8_t type)
{
    cJSON *tmp = cJSON_GetObjectItem(object, string);

    if (tmp)
    {
        if ((tmp->type & 0xFF) == type)
        {
            return tmp;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
extern void set_RGB_LED(int state);

static uint8_t http_set_led_stat(httpd_req_t *req)
{
    cJSON *root = NULL, *tmp = NULL;
    char buf[128];
    size_t len;

    httpd_post_data_t *post_data = req->post_data;

    printf("####%s:%d\r\n", __func__, __LINE__);

    printf("req->post_data:\r\n");
    printf("             size:%d\r\n", post_data->size);
    printf("             len:%d\r\n", post_data->len);
    printf("             buf:%s\r\n", post_data->buf);

    if ((post_data->len > 0) && (post_data->buf != NULL))
    {
        uint8_t sha256[32];
        sha256_hard_calculate(post_data->buf, post_data->len, sha256);
        for (uint8_t i = 0; i < 32; i++)
        {
            printf("%02X", sha256[i]);
        }
        printf("\r\n");

        if (post_data->len == (150 * 1024))
        {
            lcd_draw_picture(0, 0, 320, 240, post_data->buf);

            len = sprintf(buf, "display image ok");
            httpd_write(req, buf, len);
        }
        else
        {
            root = cJSON_Parse(post_data->buf);
            if (root)
            {
                tmp = cJSON_GetObjectItem_Type(root, "r", cJSON_Number);
                if (tmp == NULL)
                {
                    len = sprintf(buf, "get r value error");
                    httpd_write(req, buf, len);
                }
                led_stat[0] = tmp->valueint & 0x1;

                tmp = cJSON_GetObjectItem_Type(root, "g", cJSON_Number);
                if (tmp == NULL)
                {
                    len = sprintf(buf, "get g value error");
                    httpd_write(req, buf, len);
                }
                led_stat[1] = tmp->valueint & 0x1;

                tmp = cJSON_GetObjectItem_Type(root, "b", cJSON_Number);
                if (tmp == NULL)
                {
                    len = sprintf(buf, "get b value error");
                    httpd_write(req, buf, len);
                }
                led_stat[2] = tmp->valueint & 0x1;

                set_RGB_LED(led_stat);
                len = sprintf(buf, "set value success");
                httpd_write(req, buf, len);
            }
            else
            {
                len = sprintf(buf, "get value error");
                httpd_write(req, buf, len);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
