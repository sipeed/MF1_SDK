/*
  PubSubClient.cpp - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/
#include "mqtt.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
static size_t buildHeader(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    do
    {
        digit = len % 128;
        len = len / 128;
        if (len > 0)
        {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while (len > 0);

    buf[4 - llen] = header;
    for (int i = 0; i < llen; i++)
    {
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    return llen + 1; // Full header size is variable length bit plus the 1-byte
                     // fixed header
}

static uint16_t writeString(const char *string, uint8_t *buf, uint16_t pos)
{
    const char *idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp)
    {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos - i - 2] = (i >> 8);
    buf[pos - i - 1] = (i & 0xFF);
    return pos;
}

///////////////////////////////////////////////////////////////////////////////
// reads a byte into result
static uint8_t readByte0(mqtt_client_t *mqttc, uint8_t *result)
{
    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    uint32_t previousMillis = millis();

    while (tcp_client->available(tcp_client) == 0)
    {
        uint32_t currentMillis = millis();
        //FIXME:这里的超时时间是不是太大了?
        if (currentMillis - previousMillis >= ((uint32_t)MQTT_SOCKET_TIMEOUT * 1000))
        {
            return false;
        }
    }
    *result = tcp_client->read_one(tcp_client);
    return true;
}

// reads a byte into result[*index] and increments index
static uint8_t readByte(mqtt_client_t *mqttc, uint8_t *result, uint16_t *index)
{
    uint16_t current_index = *index;
    uint8_t *write_address = &(result[current_index]);
    if (readByte0(mqttc, write_address))
    {
        *index = current_index + 1;
        return true;
    }
    return false;
}

static uint16_t readPacket(mqtt_client_t *mqttc, uint8_t *lengthLength)
{
    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    uint16_t len = 0;
    if (!readByte(mqttc, mqttc->mqtt_buffer, &len))
        return 0;
    bool isPublish = (mqttc->mqtt_buffer[0] & 0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint16_t length = 0;
    uint8_t digit = 0;
    uint16_t skip = 0;
    uint8_t start = 0;

    do
    {
        if (len == 5)
        {
            // Invalid remaining length encoding - kill the connection
            mqttc->mqtt->_state = MQTT_DISCONNECTED;
            tcp_client->stop(tcp_client);
            return 0;
        }
        if (!readByte0(mqttc, &digit))
            return 0;
        mqttc->mqtt_buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier *= 128;
    } while ((digit & 128) != 0);
    *lengthLength = len - 1;

    if (isPublish)
    {
        // Read in topic length to calculate bytes to skip over for Stream writing
        if (!readByte(mqttc, mqttc->mqtt_buffer, &len))
            return 0;
        if (!readByte(mqttc, mqttc->mqtt_buffer, &len))
            return 0;
        skip = (mqttc->mqtt_buffer[*lengthLength + 1] << 8) + mqttc->mqtt_buffer[*lengthLength + 2];
        start = 2;
        if (mqttc->mqtt_buffer[0] & MQTTQOS1)
        {
            // skip message id
            skip += 2;
        }
    }

    for (uint16_t i = start; i < length; i++)
    {
        if (!readByte0(mqttc, &digit))
            return 0;

        if (len < MQTT_MAX_PACKET_SIZE)
        {
            mqttc->mqtt_buffer[len] = digit;
        }
        len++;
    }

    if (len > MQTT_MAX_PACKET_SIZE)
    {
        len = 0; // This will cause the packet to be ignored.
    }

    return len;
}

static uint8_t write(mqtt_client_t *mqttc, uint8_t header, uint8_t *buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    uint8_t *writeBuf = buf + (MQTT_MAX_HEADER_SIZE - hlen);
    uint16_t bytesRemaining = length + hlen; // Match the length type
    uint8_t bytesToWrite;
    bool result = true;
    while ((bytesRemaining > 0) && result)
    {
        int max_write_len = tcp_client->availableForWrite(tcp_client);

        bytesToWrite = (bytesRemaining > max_write_len)
                           ? max_write_len
                           : bytesRemaining;

        rc = tcp_client->write(tcp_client, writeBuf, bytesToWrite);
        result = (rc == bytesToWrite);
        bytesRemaining -= rc;
        writeBuf += rc;
    }
    return result;
}
///////////////////////////////////////////////////////////////////////////////
static void disconnect(mqtt_client_t *mqttc)
{
    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    mqttc->mqtt_buffer[0] = MQTTDISCONNECT;
    mqttc->mqtt_buffer[1] = 0;

    if (tcp_client->availableForWrite(tcp_client) >= 2)
    {
        tcp_client->write(tcp_client, mqttc->mqtt_buffer, 2);
    }
    else
    {
        printf("error @ %d\r\n", __LINE__);
    }

    mqttc->mqtt->_state = MQTT_DISCONNECTED;
    tcp_client->flush(tcp_client);
    tcp_client->stop(tcp_client);

    mqttc->lastInActivity = mqttc->lastOutActivity = millis();
}

static uint8_t connected(mqtt_client_t *mqttc)
{
    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    uint8_t rc = tcp_client->connected(tcp_client);
    if (!rc)
    {
        if (mqttc->mqtt->_state == MQTT_CONNECTED)
        {
            mqttc->mqtt->_state = MQTT_CONNECTION_LOST;
            tcp_client->flush(tcp_client);
            tcp_client->stop(tcp_client);
        }
    }

    return rc;
}

static uint8_t connect0(mqtt_client_t *mqttc, const char *id)
{
    return mqttc->connect(mqttc, id, NULL, NULL, NULL, 0, 0, 0, 1);
}

static uint8_t connect1(mqtt_client_t *mqttc,
                        const char *id, const char *user, const char *pass)
{
    return mqttc->connect(mqttc, id, user, pass, 0, 0, 0, 0, 1);
}

static uint8_t connect(mqtt_client_t *mqttc,
                       const char *id, const char *user, const char *pass,
                       const char *willTopic, uint8_t willQos,
                       uint8_t willRetain, const char *willMessage,
                       uint8_t cleanSession)
{
    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    if (mqttc->connected(mqttc) == 0)
    {
        int ret = 0;
        if (mqttc->mqtt->srv_type)
        {
            ret = tcp_client->connect_domain(tcp_client,
                                             (char *)mqttc->mqtt->srv_domain, mqttc->mqtt->srv_port);
        }
        else
        {
            ret = tcp_client->connect_domain(tcp_client,
                                             (char *)mqttc->mqtt->srv_ip, mqttc->mqtt->srv_port);
        }

        if (ret)
        {
            mqttc->nextMsgId = 1;
            // Leave room in the mqtt_buffer for header and variable length field
            uint16_t length = MQTT_MAX_HEADER_SIZE;
            unsigned int j;

#if MQTT_VERSION == MQTT_VERSION_3_1
            uint8_t d[9] = {0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 9
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
            uint8_t d[7] = {0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 7
#endif
            for (j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++)
            {
                mqttc->mqtt_buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic)
            {
                v = 0x04 | (willQos << 3) | (willRetain << 5);
            }
            else
            {
                v = 0x00;
            }
            if (cleanSession)
            {
                v = v | 0x02;
            }

            if (user != NULL)
            {
                v = v | 0x80;

                if (pass != NULL)
                {
                    v = v | (0x80 >> 1);
                }
            }

            mqttc->mqtt_buffer[length++] = v;

            mqttc->mqtt_buffer[length++] = ((mqttc->mqtt->keep_alive) >> 8);
            mqttc->mqtt_buffer[length++] = ((mqttc->mqtt->keep_alive) & 0xFF);

            CHECK_STRING_LENGTH(length, id);
            length = writeString(id, mqttc->mqtt_buffer, length);
            if (willTopic)
            {
                CHECK_STRING_LENGTH(length, willTopic);
                length = writeString(willTopic, mqttc->mqtt_buffer, length);
                CHECK_STRING_LENGTH(length, willMessage);
                length = writeString(willMessage, mqttc->mqtt_buffer, length);
            }

            if (user != NULL)
            {
                CHECK_STRING_LENGTH(length, user);
                length = writeString(user, mqttc->mqtt_buffer, length);
                if (pass != NULL)
                {
                    CHECK_STRING_LENGTH(length, pass);
                    length = writeString(pass, mqttc->mqtt_buffer, length);
                }
            }

            write(mqttc, MQTTCONNECT, mqttc->mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);

            mqttc->lastInActivity = mqttc->lastOutActivity = millis();

            while (tcp_client->available(tcp_client) == 0)
            {
                unsigned long t = millis();
                if (t - mqttc->lastInActivity >= ((int32_t)MQTT_SOCKET_TIMEOUT * 1000UL))
                {
                    mqttc->mqtt->_state = MQTT_CONNECTION_TIMEOUT;
                    tcp_client->stop(tcp_client);
                    return 0;
                }
            }

            uint8_t llen;
            uint16_t len = readPacket(mqttc, &llen);

            if (len == 4)
            {
                if (mqttc->mqtt_buffer[3] == 0)
                {
                    mqttc->lastInActivity = millis();
                    mqttc->pingOutstanding = 0;
                    mqttc->mqtt->_state = MQTT_CONNECTED;
                    return 1;
                }
                else
                {
                    mqttc->mqtt->_state = mqttc->mqtt_buffer[3];
                }
            }
            tcp_client->stop(tcp_client);
        }
        else
        {
            mqttc->mqtt->_state = MQTT_CONNECT_FAILED;
        }
        return 0;
    }
    return 1;
}

static uint8_t loop(mqtt_client_t *mqttc)
{
    uint64_t tm = millis();
    if ((tm - mqttc->last_pool) < mqttc->pool_ms)
    {
        return true;
    }
    mqttc->last_pool = tm;

    eth_tcp_client_t *tcp_client = mqttc->tcp_client;

    if (mqttc->connected(mqttc))
    {
        unsigned long t = millis();
        if (((t - mqttc->lastInActivity) > mqttc->mqtt->keep_alive * 1000UL) ||
            ((t - mqttc->lastOutActivity) > mqttc->mqtt->keep_alive * 1000UL))
        {
            if (mqttc->pingOutstanding)
            {
                mqttc->mqtt->_state = MQTT_CONNECTION_TIMEOUT;
                tcp_client->stop(tcp_client);
                return false;
            }
            else
            {
                mqttc->mqtt_buffer[0] = MQTTPINGREQ;
                mqttc->mqtt_buffer[1] = 0;
                if (tcp_client->availableForWrite(tcp_client) >= 2)
                {
                    tcp_client->write(tcp_client, mqttc->mqtt_buffer, 2);
                }
                else
                {
                    printf("error @ %d\r\n", __LINE__);
                }
                mqttc->lastOutActivity = t;
                mqttc->lastInActivity = t;
                mqttc->pingOutstanding = true;
            }
        }
        if (tcp_client->available(tcp_client))
        {
            uint8_t llen;
            uint16_t len = readPacket(mqttc, &llen);
            uint16_t msgId = 0;
            uint8_t *payload;
            if (len > 0)
            {
                mqttc->lastInActivity = t;
                uint8_t type = mqttc->mqtt_buffer[0] & 0xF0;
                if (type == MQTTPUBLISH)
                {
                    if (mqttc->mqtt->cb)
                    {
                        uint16_t tl = (mqttc->mqtt_buffer[llen + 1] << 8) + mqttc->mqtt_buffer[llen + 2]; /* topic length in bytes */
                        memmove(mqttc->mqtt_buffer + llen + 2, mqttc->mqtt_buffer + llen + 3, tl);        /* move topic inside mqtt_buffer 1 byte to front */
                        mqttc->mqtt_buffer[llen + 2 + tl] = 0;                                            /* end the topic as a 'C' string with \x00 */
                        char *topic = (char *)mqttc->mqtt_buffer + llen + 2;
                        // msgId only present for QOS>0
                        if ((mqttc->mqtt_buffer[0] & 0x06) == MQTTQOS1)
                        {
                            msgId = (mqttc->mqtt_buffer[llen + 3 + tl] << 8) + mqttc->mqtt_buffer[llen + 3 + tl + 1];
                            payload = mqttc->mqtt_buffer + llen + 3 + tl + 2;
                            mqttc->mqtt->cb((unsigned char *)topic, msgId, (unsigned char *)payload, len - llen - 3 - tl - 2);

                            mqttc->mqtt_buffer[0] = MQTTPUBACK;
                            mqttc->mqtt_buffer[1] = 2;
                            mqttc->mqtt_buffer[2] = (msgId >> 8);
                            mqttc->mqtt_buffer[3] = (msgId & 0xFF);

                            if (tcp_client->availableForWrite(tcp_client) >= 4)
                            {
                                tcp_client->write(tcp_client, mqttc->mqtt_buffer, 4);
                            }
                            else
                            {
                                printf("error @ %d\r\n", __LINE__);
                            }

                            mqttc->lastOutActivity = t;
                        }
                        else
                        {
                            payload = mqttc->mqtt_buffer + llen + 3 + tl;
                            mqttc->mqtt->cb((unsigned char *)topic, 0x0, (unsigned char *)payload, len - llen - 3 - tl);
                        }
                        memset(mqttc->mqtt_buffer, 0, MQTT_MAX_PACKET_SIZE);
                    }
                }
                else if (type == MQTTPINGREQ)
                {
                    mqttc->mqtt_buffer[0] = MQTTPINGRESP;
                    mqttc->mqtt_buffer[1] = 0;
                    if (tcp_client->availableForWrite(tcp_client) >= 2)
                    {
                        tcp_client->write(tcp_client, mqttc->mqtt_buffer, 2);
                    }
                    else
                    {
                        printf("error @ %d\r\n", __LINE__);
                    }
                }
                else if (type == MQTTPINGRESP)
                {
                    mqttc->pingOutstanding = false;
                }
            }
            else if (!tcp_client->connected(tcp_client))
            {
                // readPacket has closed the connection
                return false;
            }
        }
        return true;
    }
    return false;
}

static uint8_t publish(mqtt_client_t *mqttc,
                       const char *topic, const uint8_t *payload, unsigned int plength)
{
    if (mqttc->connected(mqttc))
    {
        if (MQTT_MAX_PACKET_SIZE < MQTT_MAX_HEADER_SIZE + 2 + strlen(topic) + plength)
        {
            // Too long
            return false;
        }
        // Leave room in the mqtt_buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, mqttc->mqtt_buffer, length);
        uint16_t i;
        for (i = 0; i < plength; i++)
        {
            mqttc->mqtt_buffer[length++] = payload[i];
        }
        uint8_t header = MQTTPUBLISH;
        if (mqttc->mqtt->retained)
        {
            header |= 1;
        }
        return write(mqttc, header, mqttc->mqtt_buffer, (length - MQTT_MAX_HEADER_SIZE));
    }
    return false;
}

static uint8_t subscribe(mqtt_client_t *mqttc,
                         const char *topic, uint8_t qos)
{
    if (qos > 1)
    {
        return false;
    }
    if (MQTT_MAX_PACKET_SIZE < 9 + strlen(topic))
    {
        // Too long
        return false;
    }
    if (mqttc->connected(mqttc))
    {
        // Leave room in the mqtt_buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        mqttc->nextMsgId++;
        if (mqttc->nextMsgId == 0)
        {
            mqttc->nextMsgId = 1;
        }
        mqttc->mqtt_buffer[length++] = (mqttc->nextMsgId >> 8);
        mqttc->mqtt_buffer[length++] = (mqttc->nextMsgId & 0xFF);
        length = writeString((char *)topic, mqttc->mqtt_buffer, length);
        mqttc->mqtt_buffer[length++] = qos;
        return write(mqttc, MQTTSUBSCRIBE | MQTTQOS1, mqttc->mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

static uint8_t unsubscribe(mqtt_client_t *mqttc, const char *topic)
{
    if (MQTT_MAX_PACKET_SIZE < 9 + strlen(topic))
    {
        // Too long
        return false;
    }
    if (mqttc->connected(mqttc))
    {
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        mqttc->nextMsgId++;
        if (mqttc->nextMsgId == 0)
        {
            mqttc->nextMsgId = 1;
        }
        mqttc->mqtt_buffer[length++] = (mqttc->nextMsgId >> 8);
        mqttc->mqtt_buffer[length++] = (mqttc->nextMsgId & 0xFF);
        length = writeString(topic, mqttc->mqtt_buffer, length);
        return write(mqttc, MQTTUNSUBSCRIBE | MQTTQOS1, mqttc->mqtt_buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

static void destory(mqtt_client_t *mqttc)
{
    if (mqttc)
    {
        free(mqttc->mqtt);
        mqttc->tcp_client->destory(mqttc->tcp_client);
        free(mqttc);
    }
}

///////////////////////////////////////////////////////////////////////////////
mqtt_t *mqtt_new(const unsigned char *domain, uint8_t *ip, uint16_t port,
                 uint8_t retained, uint16_t keep_alive,
                 mqtt_cb callback)
{
    if (!domain && !ip)
    {
        return NULL;
    }

    mqtt_t *ret = malloc(sizeof(mqtt_t));
    if (ret)
    {
        ret->_state = MQTT_DISCONNECTED;

        if (domain)
        {
            strcpy(ret->srv_domain, domain);
            ret->srv_type = 1;
        }
        else if (ip)
        {
            memcpy(ret->srv_ip, ip, 4);
            ret->srv_type = 0;
        }
        ret->srv_port = port;

        ret->retained = retained;
        ret->keep_alive = keep_alive;

        ret->cb = callback;
        return ret;
    }
    return NULL;
}
///////////////////////////////////////////////////////////////////////////////

mqtt_client_t *mqtt_client_new(mqtt_t *mqtt, uint16_t pool_ms)
{
    if (!mqtt)
    {
        return NULL;
    }

    mqtt_client_t *ret = malloc(sizeof(mqtt_client_t));
    if (ret)
    {
        ret->mqtt = mqtt;

        ret->tcp_client = eth_tcp_client_new(MAX_SOCK_NUM, TCP_CLIENT_DEFAULT_TIMEOUT);

        ret->pool_ms = pool_ms;

        ret->disconnect = disconnect;
        ret->connected = connected;

        ret->connect0 = connect0;
        ret->connect1 = connect1;
        ret->connect = connect;

        ret->loop = loop;

        ret->publish = publish;
        ret->subscribe = subscribe;
        ret->unsubscribe = unsubscribe;

        ret->destory = destory;

        return ret;
    }
    return NULL;
}
