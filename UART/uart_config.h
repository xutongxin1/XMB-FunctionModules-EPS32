#ifndef _UART_CONFIG_H_
#define _UART_CONFIG_H_
#include "UART/uart_val.h"

void uart_setup(uart_configrantion* config);
void uart_send(void* uartParameter);
void uart_rev(void* uartParameter);
#endif