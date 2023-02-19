#ifndef __UART_VAL_H__
#define __UART_VAL_H__
#include "driver/uart.h"

//TCP相关
#define TCP_BUF_SIZE 512
#define QUEUE_BUF_SIZE 64
enum CHIo
{
    CH1 = 34,
    CH2 = 25,
    CH3 = 26,

};
enum tcp_mode {
    SEND = 0,
    RECEIVE,
};
//UART_CONGFIG 串口配置参数

#define UART_BUF_SIZE 512
#define TCP_IP_ADDRESS 192, 168, 2, 171

//串口IO模式
enum UartIOMode {
    Closed = 0,//关闭
    Input,//输入,1通道可用
    Output,//输出,3通道可用
    SingleInput,//独占输入，1通道可用
    SingleOutput,//独占输出,3通道可用
    Follow1Output,//跟随1通道输出,2通道可用
    Follow3Input//跟随3通道输入,2通道可用
};

//串口引脚配置
#define RX 0
#define TX 1

struct uart_pin
{
    uint8_t CH;
    uint8_t MODE;
};

typedef struct 
{
    QueueHandle_t* buff_queue;
    struct uart_pin pin;
    uart_port_t uart_num;
    enum UartIOMode mode;
    uart_config_t uart_config;
}uart_configrantion;

typedef struct
{
    //enum Command_mode mode;
    char* buff;
    uint16_t buff_len;
}events;
typedef struct 
{
    QueueHandle_t* buff_queue;
    enum tcp_mode mode;
    enum CHIo ch;
}TcpParam;

#endif