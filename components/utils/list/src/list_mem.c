#include "list.h"

#include <string.h>
#include <stdint.h>

#include "global_config.h"
///////////////////////////////////////////////////////////////////////////////
static uint32_t list_stat[CONFIG_LIST_MAX_NUM / 32 + 1];
static uint8_t list_buf[CONFIG_LIST_MAX_NUM * sizeof(list_t)];

static uint32_t list_node_stat[CONFIG_LIST_MAX_NUM * CONFIG_LIST_NODE_MAX_NUM / 32 + 1];
static uint8_t list_node_buf[CONFIG_LIST_MAX_NUM * CONFIG_LIST_NODE_MAX_NUM * sizeof(list_node_t)];

static uint32_t list_iterator_stat[CONFIG_LIST_ITERATOR_MAX_NUM / 32 + 1];
static uint8_t list_iterator_buf[CONFIG_LIST_ITERATOR_MAX_NUM * sizeof(list_iterator_t)];
///////////////////////////////////////////////////////////////////////////////
list_t *malloc_list(void)
{
    uint32_t i, num;
    for (i = 0; i < CONFIG_LIST_MAX_NUM; i++)
    {
        if (((list_stat[i / 32] >> (i % 32)) & 0x1) == 0)
        {
            num = i;
            break;
        }
    }
    if (i >= CONFIG_LIST_MAX_NUM)
        return NULL;
    list_stat[num / 32] |= (1 << (num % 32));
    return list_buf + num * sizeof(list_t);
}

void free_list(list_node_t *list)
{
    uint32_t num = 0;
    num = ((uint32_t)list - (uint32_t)list_buf) / sizeof(list_t);
    list_stat[num / 32] &= ~(1 << (num % 32));
}
///////////////////////////////////////////////////////////////////////////////
list_node_t *malloc_list_node(void)
{
    uint32_t i, num;
    for (i = 0; i < CONFIG_LIST_MAX_NUM * CONFIG_LIST_NODE_MAX_NUM; i++)
    {
        if (((list_node_stat[i / 32] >> (i % 32)) & 0x1) == 0)
        {
            num = i;
            break;
        }
    }
    if (i >= CONFIG_LIST_MAX_NUM * CONFIG_LIST_NODE_MAX_NUM)
        return NULL;
    list_node_stat[num / 32] |= (1 << (num % 32));
    return list_node_buf + num * sizeof(list_node_t);
}
void free_list_node(list_node_t *node)
{
    uint32_t num = 0;
    num = ((uint32_t)node - (uint32_t)list_node_buf) / sizeof(list_node_t);
    list_node_stat[num / 32] &= ~(1 << (num % 32));
}

void free_all_node(void)
{
    memset(list_node_stat, 0, sizeof(list_node_stat));
}
///////////////////////////////////////////////////////////////////////////////
list_iterator_t *malloc_list_iterator(void)
{
    uint32_t i, num;
    for (i = 0; i < CONFIG_LIST_ITERATOR_MAX_NUM; i++)
    {
        if (((list_iterator_stat[i / 32] >> (i % 32)) & 0x1) == 0)
        {
            num = i;
            break;
        }
    }
    if (i >= CONFIG_LIST_ITERATOR_MAX_NUM)
        return NULL;
    list_iterator_stat[num / 32] |= (1 << (num % 32));
    return list_iterator_buf + num * sizeof(list_iterator_t);
}

void free_list_iterator(list_iterator_t *it)
{
    uint32_t num = 0;
    num = ((uint32_t)it - (uint32_t)list_iterator_buf) / sizeof(list_iterator_t);
    list_iterator_stat[num / 32] &= ~(1 << (num % 32));
}
///////////////////////////////////////////////////////////////////////////////
void list_static_init(void)
{
    memset(list_stat, 0, sizeof(list_stat));
    memset(list_node_stat, 0, sizeof(list_node_stat));
    memset(list_iterator_stat, 0, sizeof(list_iterator_stat));
}

///////////////////////////////////////////////////////////////////////////////