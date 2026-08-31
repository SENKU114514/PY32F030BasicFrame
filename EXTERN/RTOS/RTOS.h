#ifndef __RTOS_H__
#define __RTOS_H__

#include "main.h"

/*单函数跳转流程：进入函数后LR寄存器会自动+1储存地址，之后bx回去地址
 *嵌套函数跳转流程：进入函数后，压入当前lr寄存器(最外层地址),之后按照单函数跳转，返回时弹栈给PC实现回调
 *汇编跳转流程：当函数在.s被声明的时候，跳转进汇编的时候后面的参数会被储存在R0-R7中，并且进入时LR记录自身地址+1
*/



//记录任务上下文结构体
typedef struct {
	uint32_t return_addr;//返回地址，返回回函数的地址
	uint32_t stack_addr;//栈指针
}task_context_t;

extern task_context_t task0,task1;//声明并占用这个名字，还需要在.c再定义一次

void RTOS_init(void);	//初始化结构体，传入相关参数

void task_0(void);										//
void task_1(void);										//
void task_switch(task_context_t * from, task_context_t * to);		//任务切换，传入地址参数
void task_run_first(task_context_t * to);

#endif



