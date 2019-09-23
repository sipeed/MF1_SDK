#include "demo_w5500.h"

#include "fpioa.h"
#include "gpiohs.h"

#include "board.h"

#include "spi/myspi.h"

#include "Ethernet.h"
#include "EthernetClient.h"
#include "EthernetServer.h"

#include "httpd.h"
#include "webpage.h"

#include "http_file.h"
#include "mqtt.h"

#include "global_config.h"

#if CONFIG_NET_ENABLE
#if CONFIG_NET_W5500

uint8_t http_header[1024];

static mqtt_client_t *mqttc = NULL;
static unsigned char *mqtt_topic = "/public/TEST/SIPEED";

static eth_tcp_server_t *server = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint16_t last_msgId = -1;
static void mqtt_callback(unsigned char *intopic, uint16_t msgId, unsigned char *payload, unsigned int length)
{
    *(payload + length) = 0;

#if 1
    printk("topic: %s \r\n", intopic);
    printk("msgId: %d \r\n", msgId);
    printk("payload len: %d \r\n", length);
    printk("payload: %s\r\n\r\n", payload);
#endif

    if (last_msgId != msgId)
    {
        last_msgId = msgId;
    }
    else
    {
        printk("same msgId\r\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FIXME: 如果服务器掉线，不能一直等待。。。
static void mqtt_reconnect(void)
{
    while (!mqttc->connected(mqttc))
    {
        printk("Attempting MQTT connection...\r\n");
        if (mqttc->connect0(mqttc, "XELLLLLL"))
        {
            printk("reconnected\r\n");
            mqttc->publish(mqttc, mqtt_topic, "reconnected", strlen("reconnected"));
            mqttc->subscribe(mqttc, mqtt_topic, 1);
        }
        else
        {
            printk("failed, rc=%d\r\n", mqttc->mqtt->_state);
        }
    }
}

void demo_w5500(void)
{
    uint64_t tt, tm;
    float time_s, file_kb;

    uint8_t mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

    printf("demo_w5500\r\n");

    printk("W5500:\r\n");
    printf("      MOSI:%d\r\n", CONFIG_ETH_PIN_MOSI);
    printf("      MISO:%d\r\n", CONFIG_ETH_PIN_MISO);
    printf("      SCLK:%d\r\n", CONFIG_ETH_PIN_SCLK);
    printf("        CS:%d\r\n", CONFIG_ETH_PIN_CS);
    // printf("      RST:%d\r\n", CONFIG_ETH_PIN_RST);

    /* 和继电器分配的IO有冲突 */
    fpioa_set_function(CONFIG_ETH_PIN_CS, FUNC_GPIOHS0 + CONFIG_ETH_GPIOHS_NUM_CS); //CSS
    fpioa_set_function(CONFIG_ETH_PIN_MISO, FUNC_SPI1_D1);                          //MISO
    fpioa_set_function(CONFIG_ETH_PIN_MOSI, FUNC_SPI1_D0);                          //MOSI
    fpioa_set_function(CONFIG_ETH_PIN_SCLK, FUNC_SPI1_SCLK);                        //CLK

    //初始化spi
    eth_w5500_spi_init();
    //注册回调
    W5500_reg_cs_func(eth_w5500_spi_cs_sel, eth_w5500_spi_cs_desel); //注册SPI片选信号函数
    W500_reg_spi_func(eth_w5500_spi_write, eth_w5500_spi_read);      //注册读写函数

    if (eth_w5100_begin_dhcp(mac, 60000, 4000) == 0)
    {
        printf("Failed to configure Ethernet using DHCP\r\n");

        if (eth_w5100_hardwareStatus() == EthernetNoHardware)
        {
            printf("Ethernet shield was not found.  Sorry, can't run without hardware. :(\r\n");
        }
        else if (eth_w5100_linkStatus() == LinkOFF)
        {
            printf("Ethernet cable is not connected.\r\n");
        }
        else
        {
            printf("unknown status\r\n");
        }
        //这里DHCP获取IP失败了。
        while (1)
        {
            uint32_t cnt = 0;
            cnt++;
            if (cnt > 0x1ff)
            {
                cnt = 0;
                printf("w5500 dhcp failed\r\n");
            }
        }
    }
    IPAddress ip;

    printf("init over\r\n");
    ip = eth_w5100_localIP();
    printf("eth_w5100_localIP: %d:%d:%d:%d\r\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);

    ip = eth_w5100_subnetMask();
    printf("eth_w5100_subnetMask: %d:%d:%d:%d\r\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);

    ip = eth_w5100_gatewayIP();
    printf("eth_w5100_gatewayIP: %d:%d:%d:%d\r\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);

    ip = eth_w5100_dnsServerIP();
    printf("eth_w5100_dnsServerIP: %d:%d:%d:%d\r\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);

#if HTTP_GET_DEMO
    printf("HTTP_GET_DEMO\r\n");
    tm = sysctl_get_time_us();
    //测速结果
    //外网下载大概在500K/s左右
    //内网下载大概在2-3M/s左右
    uint32_t get_recv_len = http_get_file(
        HTTP_GET_URL,
        NULL,
        http_header,
        sizeof(http_header),
        display_image,
        sizeof(display_image));

    tt = sysctl_get_time_us() - tm;

    printk("http get use %ld us\r\n", tt);

    time_s = (float)((float)tt / 1000.0 / 1000.0);
    file_kb = (float)((float)get_recv_len / 1024.0);

    printf("about %f KB/s\r\n", (float)(file_kb / time_s));

    printk("get_recv_len:%d\r\n", get_recv_len);
    printk("get recv hdr:%s\r\n", http_header);
#endif /* HTTP_GET_DEMO */

#if HTTP_POST_DEMO
    printf("HTTP_POST_DEMO\r\n");
    uint8_t *post_send_body = (uint8_t *)malloc(sizeof(uint8_t) * 1024);
    uint8_t *post_send_header = (uint8_t *)malloc(sizeof(uint8_t) * 512);

    uint8_t *boundary = "----WebKitFormBoundaryO2aA3WiAfUqIcD6e"; //header 中要比正文少俩--
                                                                  //结尾又比正文在街位数多俩--
    sprintf(post_send_header, "Sec-Fetch-Mode: cors\r\nUser-Agent: Xel\r\nContent-Type: multipart/form-data; boundary=%s\r\n", boundary);
    sprintf(post_send_body, "--%s\r\nContent-Disposition: form-data; name=\"file\"\r\n\r\nmultipart\r\n--%s\r\nContent-Disposition: form-data; name=\"Filedata\"; filename=\"logo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n", boundary, boundary);

    tm = sysctl_get_time_us();
    uint32_t post_recv_len = http_post_file("http://api.uomg.com/api/image.ali",
                                            post_send_header,
                                            post_send_body,
                                            boundary,
                                            display_image,
                                            get_recv_len,
                                            http_header,
                                            sizeof(http_header),
                                            display_image,
                                            sizeof(display_image));

    tt = sysctl_get_time_us() - tm;

    if (post_recv_len > 0)
    {
        printk("post_recv_len %d \r\n", post_recv_len);
        printk("http_header:%s\r\n", http_header);

        time_s = (float)((float)tt / 1000.0 / 1000.0);
        file_kb = (float)((float)get_recv_len / 1024.0);

        printk("http post use %ld us\r\n", tt);

        printf("about %f KB/s\r\n", (float)(file_kb / time_s));
    }
#endif /* HTTP_POST_DEMO */

#if MQTT_DEMO
    printf("MQTT_DEMO\r\n");
    static uint8_t *mqtt_ip = NULL;
    static unsigned char *mqtt_domain = "mq.tongxinmao.com";
    static uint16_t mqtt_port = 18830;

    mqtt_t *mqtt = mqtt_new(mqtt_domain, mqtt_ip, mqtt_port, 0, 60, mqtt_callback);

    if (mqtt)
    {
        mqttc = mqtt_client_new(mqtt, 100);
        if (mqttc)
        {
            if (mqttc->connect0(mqttc, "XELLLLLL"))
            {
                printk("mqtt connect successed\r\n");
                mqttc->publish(mqttc, mqtt_topic, "hello world", strlen("hello world"));
                mqttc->subscribe(mqttc, mqtt_topic, 1);
            }
            else
            {
                printk("mqtt connect failed\r\n");
            }
        }
        else
        {
            printf("AAAAA\r\n");
        }
    }
    else
    {
        printf("AAAA\r\n");
    }
#endif /* MQTT_DEMO */

#if HTTPD_DEMO
    printf("HTTPD_DEMO\r\n");
    webpage_init_size();
    server = eth_tcp_server_new(80);
    if (server)
    {
        printf("server init\r\n");
        server->begin(server);
    }
#endif /* HTTPD_DEMO */

    while (1)
    {

#if HTTPD_DEMO
        if (server)
        {
            eth_tcp_client_t *srv_client = server->available(server);

            if (srv_client)
            {
                printf("new client\r\n");
                httpd_handle_request(srv_client);
            }
        }
#endif /* HTTPD_DEMO */

#if MQTT_DEMO
        {
            /* mqtt loop */
            if (mqttc->loop(mqttc) == false)
            {
                /* disconnect */
                mqtt_reconnect();
            }
        }
#endif /* MQTT_DEMO */

        switch (eth_w5100_maintain())
        {
        case 1:
            //renewed fail
            printf("Error: renewed fail\r\n");
            break;

        case 2:
            //renewed success
            printf("Renewed success\r\n");
            //print your local IP address:
            ip = eth_w5100_localIP();
            printf("eth_w5100_localIP: %d:%d:%d:%d\r\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);
            break;

        case 3:
            //rebind fail
            printf("Error: rebind fail\r\n");
            break;

        case 4:
            //rebind success
            printf("Rebind success\r\n");
            //print your local IP address:
            ip = eth_w5100_localIP();
            printf("eth_w5100_localIP: %d:%d:%d:%d\r\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);
            break;

        default:
            //nothing happened
            break;
        }
    }
}
#endif /* CONFIG_NET_W5500 */
#endif /* CONFIG_NET_ENABLE */