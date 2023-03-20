#ifndef __UART_ANALYSIS_PARAMETERS_H__
#define __UART_ANALYSIS_PARAMETERS_H__

#include "UART/uart_val.h"


int uart_1_parameter_analysis(void *attach_rx_buffer,uart_init_t* uartconfig);
int uart_2_parameter_analysis(void *attach_rx_buffer,uart_init_t* uartconfig); 

#endif