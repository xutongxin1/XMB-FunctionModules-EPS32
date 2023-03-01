#ifndef _TCP_H_
#define _TCP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
//TCP相关
#define TCP_BUF_SIZE 512
#define QUEUE_BUF_SIZE 64
// enum CHIo
// {
//     CH1 = 34,
//     CH2 = 25,
//     CH3 = 26,

// };

enum Port
{
    CH1 = 1920,
    CH2,
    CH3,
    CH4


};

enum tcp_mode {
    SEND = 0,
    RECEIVE,
    ALL,
};

typedef struct 
{
    QueueHandle_t* tx_buff_queue;
    QueueHandle_t* rx_buff_queue;
    enum tcp_mode mode;
    enum Port port;
}TcpParam;
typedef struct 
{
    TcpParam* TcpParam;
    struct netconn *conn;
    struct netconn *newconn;
    uint8_t* task_flag;
}SubTcpParam;
typedef struct 
{
    TaskHandle_t** TaskHandle[10];
    uint8_t TaskNum;
}TcpTaskHandle_t;
void tcp_send_server(void *Parameter);
void tcp_rev_server(void *Parameter);
void tcp_server(void *Parameter);
void tcp_test_server(void *Parameter);
#endif