#include "SERVER/IR/IR_Receive.h"
#include "main.h"
#include <string.h>

//static uint8_t Logical_Zero = 1;	//红外逻辑0
//static uint8_t Logical_One = 2;		//红外逻辑1

static TIM_TypeDef *TIMx;					//计时器
static GPIO_index_e GPIO_INDEX;		//引脚

static uint8_t IR_Receive_Byte = 4;//默认接收四个字节		

/*修改接收数据量,单位:字节*/
void Server_IR_Change_ReceiveNum(uint8_t num){
	IR_Receive_Byte = num;
}

/*初始化IR获取(下降沿)*/
SERVER_IR_Status_e Server_IR_init(TIM_TypeDef *TIMx_init,GPIO_index_e GPIO_index_init){
	
	TIMx = TIMx_init;
	GPIO_INDEX = GPIO_index_init;
	
	HW_GPIO_Status_e status;
	status = HW_GPIO_INPUT_IT_init(GPIO_INDEX, UP, GPIO_IT_FALLING);	//设置上拉,下拉捕获
	if(status != HW_GPIO_STATUS_OK){
		return SERVER_IR_IT_INIT_FAIL;
	}
	HW_TIM_TIME_MS_init(TIMx);																				//初始化计数模式
	
	return SERVER_IR_OK;
}

#define IR_BuffNum 256U								//数量上限
static uint32_t IR_Info[IR_BuffNum];	//原始数据存放地
static uint32_t time = 0; 						//中断计时时间缓存
static uint16_t IR_index = 0;					//原始数据个数

static uint8_t IR_Receive_flag = 0;		//IR数据满足数量标志位

/*IO中断位置(注释防止冲突)*/
//void HW_GPIO_INPUT_IT_Callback(GPIO_index_e GPIO_index)
//{
//	//中断捕获后获取下降时间间隔
//	if(GPIO_index == GPIO_INDEX){
//		HW_TIM_TIME_MS_Stop(TIMx, &time);//停止并输出参数
//		
//		if(time > 13){
//			time = 0;//预防间隔过大导致无法识别数据
//		}
//		
//		if(time != 0){
//			IR_Info[IR_index] = time;	//记录时间
//			IR_index++;								//记录数量
//			time = 0;
//		}
//		
//		if(IR_index >= 1+IR_Receive_Byte*8){
//			IR_Receive_flag = 1;			//设计接收标志位
//		}
//		HW_TIM_TIME_MS_Start(TIMx);	//清零计时后计时
//	}
//}

/*清除缓存*/
void IR_Receive_Clearn(void){
	IR_Receive_flag = 0;//处理后清零
	time = 0;						//清除计时器
	for(uint8_t i=0;i<IR_index;i++){
		IR_Info[i] = 0;
	}
	IR_index = 0;				//清除数量
}

static uint8_t IR_Byte[32] = {0}; //字节存放位置
/*处理原始数据转换为字节*/
void Server_IR_Data_Handle(void *context){
	
	//判断接收标志位完成
	if(IR_Receive_flag){
		if(IR_Info[0] == 13){//判断头是否正确
			
			//处理原始数据
			for(uint8_t i=0;i<IR_Receive_Byte;i++){
				uint8_t Data=0;
				for(uint8_t j=0;j<8;j++){
					Data |= ((IR_Info[i*8+j+1]-1) << j);
				}
				IR_Byte[i] = Data;
			}
			
			IR_Receive_Clearn();//清除缓存
		}
		else{
			IR_Receive_Clearn();//清除缓存
		}
	}
}

/*修改接收长度*/
void Server_IR_ChangeReceiveLen(uint8_t num){
	IR_Receive_Clearn();			//防止数据错乱(主动变为不合法数据)
	IR_Receive_Byte = num;		//修改接收数量
}

/*返回接收字节*/
void Server_IR_Data_Return(uint8_t *arry, uint8_t *num){
	uint8_t i;
	*num = IR_Receive_Byte;

	for(i = 0U; i < IR_Receive_Byte; i++)
	{
			arry[i] = IR_Byte[i]; //把全局IR_Byte每一字节写入外部data[]
	}
}



