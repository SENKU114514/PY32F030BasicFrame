#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define QUEUE_SIZE 64

typedef enum
{
	SERVER_STATUS_OK,
	SERVER_STATUS_FULL,
	SERVER_STATUS_ABOVE_LIMIT,
	SERVER_STATUS_NO_DATA,
	SERVER_STATUS_INIT_FAIL,
}SERVER_STATUS_Queue_e;

typedef struct
{
		//ÃÊªª≥…Õ‚≤ø¥Ê¥¢
    uint8_t *buffer;
    uint16_t size;
	
    uint16_t read_index;
    uint16_t write_index;
    uint16_t count;
}Queue;

#endif

