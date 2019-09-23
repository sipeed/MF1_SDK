/*
 PubSubClient.h - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/
#ifndef __MQTT_H
#define __MQTT_H

#include <stdint.h>

#include "utility/w5100.h"
#include "EthernetClient.h"

/* clang-format off */

#define DOMAIN_MAX_LEN                (255)

#define MQTT_VERSION_3_1              (3)
#define MQTT_VERSION_3_1_1            (4)

// MQTT_VERSION : Pick the version
//#define MQTT_VERSION MQTT_VERSION_3_1
#ifndef MQTT_VERSION
#define MQTT_VERSION                  (MQTT_VERSION_3_1_1)
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE          (4 * 1024)
#endif

// MQTT_KEEPALIVE : keepAlive interval in Seconds
// #ifndef MQTT_KEEPALIVE
// #define MQTT_KEEPALIVE 15
// #endif

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT           (15)
#endif

// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT       (-4)
#define MQTT_CONNECTION_LOST          (-3)
#define MQTT_CONNECT_FAILED           (-2)
#define MQTT_DISCONNECTED             (-1)
#define MQTT_CONNECTED                ( 0)
#define MQTT_CONNECT_BAD_PROTOCOL     ( 1)
#define MQTT_CONNECT_BAD_CLIENT_ID    ( 2)
#define MQTT_CONNECT_UNAVAILABLE      ( 3)
#define MQTT_CONNECT_BAD_CREDENTIALS  ( 4)
#define MQTT_CONNECT_UNAUTHORIZED     ( 5)

#define MQTTCONNECT                   (1 << 4 ) // Client request to connect to Server
#define MQTTCONNACK                   (2 << 4 ) // Connect Acknowledgment
#define MQTTPUBLISH                   (3 << 4 ) // Publish message
#define MQTTPUBACK                    (4 << 4 ) // Publish Acknowledgment
#define MQTTPUBREC                    (5 << 4 ) // Publish Received (assured delivery part 1)
#define MQTTPUBREL                    (6 << 4 ) // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP                   (7 << 4 ) // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE                 (8 << 4 ) // Client Subscribe request
#define MQTTSUBACK                    (9 << 4 ) // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE               (10 << 4) // Client Unsubscribe request
#define MQTTUNSUBACK                  (11 << 4) // Unsubscribe Acknowledgment
#define MQTTPINGREQ                   (12 << 4) // PING Request
#define MQTTPINGRESP                  (13 << 4) // PING Response
#define MQTTDISCONNECT                (14 << 4) // Client is Disconnecting
#define MQTTReserved                  (15 << 4) // Reserved

#define MQTTQOS0                      (0 << 1)
#define MQTTQOS1                      (1 << 1)
#define MQTTQOS2                      (2 << 1)

// Maximum size of fixed header and variable length size header
#define MQTT_MAX_HEADER_SIZE          (5)

#define CHECK_STRING_LENGTH(l, s)                 \
  do                                              \
  {                                               \
    if (l + 2 + strlen(s) > MQTT_MAX_PACKET_SIZE) \
    {                                             \
      tcp_client->stop(tcp_client);               \
      return false;                               \
    }                                             \
  } while (0)

/* clang-format on */

typedef void (*mqtt_cb)(unsigned char *topic, uint16_t msg_id, unsigned char *payload, unsigned int length);

typedef struct _mqtt
{
  uint8_t srv_type; // 1 doamin, 0 ip

  uint8_t srv_ip[4];
  unsigned char srv_domain[DOMAIN_MAX_LEN];
  uint16_t srv_port;

  uint8_t retained;
  uint16_t keep_alive;

  int8_t _state;

  mqtt_cb cb;
} mqtt_t;

typedef struct _mqtt_client mqtt_client_t;

typedef struct _mqtt_client
{
  mqtt_t *mqtt;
  eth_tcp_client_t *tcp_client;

  uint16_t nextMsgId;
  unsigned long lastOutActivity;
  unsigned long lastInActivity;
  uint8_t pingOutstanding;

  uint8_t mqtt_buffer[MQTT_MAX_PACKET_SIZE];

  uint16_t pool_ms; //100 * 1000
  uint64_t last_pool;
  ///////////////////////////////////////////////////////////////////////////////
  uint8_t (*loop)(mqtt_client_t *mqttc);

  void (*disconnect)(mqtt_client_t *mqttc);
  uint8_t (*connected)(mqtt_client_t *mqttc);

  uint8_t (*connect0)(mqtt_client_t *mqttc, const char *id);

  uint8_t (*connect1)(mqtt_client_t *mqttc,
                      const char *id, const char *user, const char *pass);

  uint8_t (*connect)(mqtt_client_t *mqttc, const char *id, const char *user, const char *pass,
                     const char *willTopic, uint8_t willQos,
                     uint8_t willRetain, const char *willMessage,
                     uint8_t cleanSession);

  uint8_t (*publish)(mqtt_client_t *mqttc,
                     const char *topic,
                     const uint8_t *payload,
                     unsigned int plength);

  uint8_t (*subscribe)(mqtt_client_t *mqttc, const char *topic, uint8_t qos);

  uint8_t (*unsubscribe)(mqtt_client_t *mqttc, const char *topic);

  void (*destory)(mqtt_client_t *mqttc);
} mqtt_client_t;

mqtt_t *mqtt_new(const unsigned char *domain, uint8_t *ip, uint16_t port,
                 uint8_t retained, uint16_t keep_alive,
                 mqtt_cb callback);

mqtt_client_t *mqtt_client_new(mqtt_t *mqtt, uint16_t pool_ms);

#endif
