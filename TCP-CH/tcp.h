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
    SEND = 1,
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
    struct netconn **conn;
    struct netconn **newconn;
    uint8_t taskcount;
}SubTcpParam;

typedef struct 
{
    TaskHandle_t* FatherTaskHandle;
    uint8_t FatherTaskcount;
    enum Port FatherTaskPort;
    TaskHandle_t* SonTaskHandle;
    uint8_t SonTaskcount;
    char SonTaskname[16];
    bool FatherTask_exists;
    bool SonTask_exists;
    enum tcp_mode mode;
}TcpTaskHandle_t;

void tcp_send_server(void *Parameter);
void tcp_rev_server(void *Parameter);
void tcp_server(void *Parameter);
void tcp_test_server(void *Parameter);
uint8_t create_tcp_server(uint16_t port, struct netconn **conn);
uint8_t TcpTaskAllDelete(TcpTaskHandle_t* TCP_TASK_HANDLE_delete);
TcpTaskHandle_t* TcpTaskCreate(void *Parameter);
#endif