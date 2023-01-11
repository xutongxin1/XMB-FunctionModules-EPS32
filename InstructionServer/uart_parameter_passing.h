#ifndef __UART_PARAMETER_PASSING_H__
#define __UART_PARAMETER_PASSING_H__

typedef struct uart_configrantion
{
    QueueHandle_t* uart_queue;
    struct uart_pin pin;
    uart_port_t uart_num;
    enum UartIOMode mode;
    uart_config_t uart_config;
};

struct uart_pin
{
    uint8_t CH;
    uint8_t MODE;
};

//void uart_parameter_passing(struct Uart_parameter_Analysis* Uart_parameter,struct uart_configrantion* uart_config);