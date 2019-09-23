
#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

/* specify default page */
static char config_default_page[] = "index.html";

typedef struct _err_html
{
    int err_code;
    char *fname;
} err_html_t;

static err_html_t err_pages[] = {
    // {404, "404.html"},
    // {500, "500.html"},
    {0, NULL},
};

#endif //WEB_CONFIG_H
