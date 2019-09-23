#ifndef __DEMO_W5500_H
#define __DEMO_W5500_H

#include "global_config.h"

#if CONFIG_NET_ENABLE
#if CONFIG_NET_W5500

/* clang-format off */

/*speed test, get 150K */
#define HTTP_GET_URL "http://119.145.88.166:8080/download?nocache=ab86f05e-1195-43a6-8685-8c78f2a70ecf&size=102400"

#define HTTP_GET_DEMO       (1)
#define HTTP_POST_DEMO      (1)

#define MQTT_DEMO           (1)

#define HTTPD_DEMO          (1)

/* clang-format on */

#endif
#endif
#endif
