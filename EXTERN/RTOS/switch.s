	;采用ARM语法
	;原理：每次由代码跳转进汇编的时候，
		;R0会储存我们函数传入的第一个参数
		;R1会储存第二个参数，后面同理
		;LR寄存器会自动储存当前指令地址+1
	
	AREA CODE_SEG,CODE,READONLY;内存分段存flash,代码段,只读
	EXPORT task_switch			;函数全局声明
	EXPORT task_run_first		;函数全局声明
		
task_switch		;跳转函数
	;r0->stack_addr <= sp,将sp参数获取进stack_addr中
	MOV R3, SP			;取出SP
	STR R3, [R0, #4]	;将SP存入结构体stack_addr中
	
	;r0->return_addr <= lr
	MOV R3, LR          ;取出LR数值
	STR R3, [R0, #0]    ;将取出的数值存入结构体return_addr中
	
	;sp <= r1->stack_addr
	LDR R3, [R1, #4]	;取出结构体指针第二个内容(基地址)
	MOV SP, R3		;将参数传入SP中
	
	;PC <= r1->return_addr
	LDR R3, [R1, #0]	;取出结构体指针第一个内容
	BX R3				;将取出的内容输出到PC，也就是任务地址,与MOV PC, R3一样的效果

task_run_first
	;将相关任务栈的地址传入栈(sp)地址中
	LDR R3, [R0, #4]	;取出结构体指针第二个内容(基地址)
	MOV SP, R3		;将参数传入SP中
	
	;将程序跳转到相关任务(PC)
	LDR R3, [R0, #0]	;取出结构体指针第一个内容
	BX R3				;将取出的内容输出到PC，也就是任务地址,与MOV PC, R3一样的效果
	
	
	
	END
