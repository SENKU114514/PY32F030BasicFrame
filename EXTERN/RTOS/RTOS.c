/*任务切换，还没有调度器*/

/*跳转逻辑：
 *	->初始化参数RTOS_init();，内部传入任务相关函数地址与栈空间大小
 *	->task_run_first(&task0);通过函数进入汇编
 *		-->跳转进任务，将结构体参数(栈地址,返回地址)分别给SP与PC地址
 *	->进入任务task0运行直到task_switch(&task0, &task1),参数分别为记录task0 SP(栈指针)与LR(函数返回地址)，跳转进task1的 SP(栈空间)与LR(返回地址)
 *	->运行taks1运行直到task_switch(&task1, &task0)参数分别为记录task0 SP(栈指针)与LR(函数返回地址)，跳转进task1的 SP(栈空间)与LR(返回地址)
 *	->直到结尾，他会推出函数之前的压栈，然后返回原本的程序位置
 *
 *
 */

#include "RTOS.h"

uint32_t task0_stack[80];
uint32_t task1_stack[80];

task_context_t task0,task1;

void RTOS_init()
{
	//传入初始化地址
	task0.return_addr = (uint32_t)task_0;
	task1.return_addr = (uint32_t)task_1;
	
	//传入初始栈空间
	task0.stack_addr = (uint32_t)&task0_stack[80];
	task1.stack_addr = (uint32_t)&task1_stack[80];
}

void task_0()
{
	int i,j,k,l,m,n,o,p,q = 0;
	i++;
	j++;
	k++;
	l++;
	m++;
	n++;
	o++;
	p++;
	q++;
	i++;
	
	//task1();
	task_switch(&task0, &task1);//传入任务地址
	k++;
	l++;
	task_switch(&task0, &task1);//传入任务地址
}
void task_1()
{
	int j = 0;
	j++;
	j++;
	//task0();
	task_switch(&task1, &task0);//传入任务地址
	j++;
	j++;
	task_switch(&task1, &task0);//传入任务地址
}


