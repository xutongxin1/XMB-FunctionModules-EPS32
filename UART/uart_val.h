#ifndef __UART_VAL_H__
#define __UART_VAL_H__
#include "driver/uart.h"

//UART_CONGFIG 串口配置参数

#define UART_BUF_SIZE 512
#define TCP_IP_ADDRESS 192, 168, 199, 202

#define EVENT_BUFF_SIZE 512

//串口IO模式
enum UartIOMode {
   Send,    //单发
   Receive, //单收
   Forward, //转发
   All,     //全双工
};

//串口引脚配置
#define RX 0
#define TX 1

struct uart_pin
{
    uint8_t rx_pin;
    uint8_t tx_pin;
};

typedef struct 
{
    QueueHandle_t* rx_buff_queue;
    QueueHandle_t* tx_buff_queue;
    QueueHandle_t* forward_buff_queue;
    struct uart_pin pin;
    uart_port_t uart_num;
    enum UartIOMode mode;
    uart_config_t uart_config;
}uart_init_t;

typedef struct
{
    //enum Command_mode mode;
    char buff_arr[EVENT_BUFF_SIZE];
    char* buff;
    uint16_t buff_len;
}events;

typedef enum
{
    UART_OK = 0,
    UART_ERROR = -1,
    UART_NUM_EXISTED = -2,
    UART_SET_PAIN_FAIL = -3,
    UART_CONFIG_FAIL = -4,
    UART_INSTALL_FAIL = -5,
    UART_REGISTER_FAIL = -6,
}uart_err_t;



typedef struct
{
    uint8_t rx_pin;
    uint8_t tx_pin;
    uint16_t baud_rate;
    uint8_t data_bit;
    uint8_t parity_bit;
    uint8_t stop_bit;
}uart_state_t;

typedef struct
{
    uint8_t task_num;
    uart_port_t port;
    TaskHandle_t* handle[2];
}uart_task_t;

typedef struct
{
    uart_port_t existed_port[3];
    uart_state_t state[3];
    uint8_t existed_num;
    uint8_t task_num;
    uart_task_t task_handle[3];
}uart_manage_t;



#endif