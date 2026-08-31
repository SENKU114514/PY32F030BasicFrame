#include "./SERVER/LOG/LOG.h"

#define Uart_Rate 115200U

Log_State_e Log_init(void){
	
	HW_UART_Status_e status = HW_UART_STATUS_OK;
	
	status = HW_UART_init(UARTx, TX_pin, RX_pin, Uart_Rate);
	
	if(status != HW_UART_STATUS_OK){
		return LOG_FAIL;
	}
	
	LOG_DEBUG("LOG init success\r\n");
	return LOG_OK;
}



