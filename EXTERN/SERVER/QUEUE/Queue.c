#include "./SERVER/QUEUE/Queue.h"

//初始化内部数据
void Queue_Init(Queue *q,uint8_t *buffer,uint16_t size)
{
		//输入储存队列
		q->buffer = buffer;
    q->size = size;
	
		//设定默认值
    q->read_index = 0;
    q->write_index = 0;
    q->count = 0;
}

//推入缓存区
SERVER_STATUS_Queue_e Queue_Push(Queue *q, uint8_t data)
{
		//数量大于设定则返回
    if (q->count >= QUEUE_SIZE)
        return SERVER_STATUS_ABOVE_LIMIT;
		
		//添加数据
    q->buffer[q->write_index] = data;
    q->write_index++;
		
		//设定写入索引最大值
    if (q->write_index >= QUEUE_SIZE)
        q->write_index = 0;
		
    q->count++;

    return SERVER_STATUS_OK;
}

//推出
SERVER_STATUS_Queue_e Queue_Pop(Queue *q, uint8_t *data)
{
		//限定数量
    if (q->count == 0)
        return SERVER_STATUS_NO_DATA;

		//推出数据
    *data = q->buffer[q->read_index];
    q->read_index++;
		
		//设定读取索引最大值
    if (q->read_index >= QUEUE_SIZE)
        q->read_index = 0;

    q->count--;

    return SERVER_STATUS_OK;
}



