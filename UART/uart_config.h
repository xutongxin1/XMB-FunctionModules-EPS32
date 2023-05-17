#ifndef _UART_CONFIG_H_
#define _UART_CONFIG_H_
#include "UART/uart_val.h"

uart_err_t Delete_All_Uart_Task(void);
uart_err_t Create_Uart_Task(uart_init_t *Parameter);
uart_err_t uart_setup(uart_init_t *config);
uart_err_t is_uart_num_free(uart_port_t uart_num);
uart_port_t get_uart_free_num(void);
uart_err_t uart_state_register(uart_init_t *config);
uint8_t get_uart_manage_id(uart_port_t uart_num);
uart_err_t uart_setup(uart_init_t *config);
void uart_send(uart_init_t* uartParameter);
void uart_rev(uart_init_t* uartParameter);
#endif