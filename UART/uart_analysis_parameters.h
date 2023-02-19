#ifndef __UART_ANALYSIS_PARAMETERS_H__
#define __UART_ANALYSIS_PARAMETERS_H__

#include "UART/uart_val.h"

uart_configrantion c1;
uart_configrantion c2;
uart_configrantion c3;

int uart_1_parameter_analysis(void *attach_rx_buffer, uart_configrantion* t) ;
int uart_2_parameter_analysis(void *attach_rx_buffer, uart_configrantion* t);

#endif