
#include "sdkconfig.h"

#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdatomic.h>

#include "InstructionServer/wifi_configuration.h"
#include "UART/uart_val.h"
#include "TCP-CH/tcp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netbuf.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "lwip/priv/api_msg.h"
#include <lwip/netdb.h>

#define EVENTS_QUEUE_SIZE 10

#ifdef CALLBACK_DEBUG
#define debug(s, ...) printf("%s: " s "\n", "Cb:", ##__VA_ARGS__)
#else
#define debug(s, ...)
#endif
#define MY_IP_ADDR(...) IP4_ADDR(__VA_ARGS__)
static const char *TCP_TAG = "TCP";
static err_t get_connect_state(struct netconn *conn);
static TaskHandle_t SubTask_Handle;
static TaskHandle_t SubTask_Handle_1;
static TaskHandle_t TCP_TASK_HANDLE[10];
static uint8_t subtask_one_flag = 0;
//static TaskHandle_t** TcpTaskHandle;
static TcpTaskHandle_t TcpHandle = {
        .TaskNum = 0,
    };
    
// static uint16_t choose_port(uint8_t pin)
// {
//     switch (pin)
//     {
//     case CH2:
//         return 1922;
//     case CH3:
//         return 1923;
//     case CH1:
//         return 1921;
//     default:
//         return -1;
//     }
// }
/**
 * @ingroup xmb tcp server
 * Create a tcp server according to the configuration parameters
 * It is an internal function
 * @param Parameter the configuration parameters
 * @param conn A netconn descriptor 
 * @return ERR_OK if bound, any other err_t on failure
 */
static uint8_t create_tcp_server(uint16_t port, struct netconn** conn)
{
    while (*conn == NULL)
    {
        *conn = netconn_new(NETCONN_TCP);
    }
    printf("creat : %p\r\n",*conn);
    // netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    //netconn_bind(*conn, &ip_info.ip, Param->port);
     /* Bind connection to well known port number 7. */
    netconn_bind(*conn, IP_ADDR_ANY, port);
    netconn_listen(*conn); /* Grab new connection. */
    printf("PORT: %d\nLISTENING.....\n", port);
    return ESP_OK;
}

void tcp_send_server(void *Parameter)
{
    // printf("Parameter = %p  \n", Parameter);
    TcpParam *Param = (TcpParam *)Parameter;
    // printf("Param = %p  \n", Param);
    struct netconn *conn = NULL;
    // printf("conn\n");
    struct netconn *newconn = NULL;

    err_t err = 1;
    // uint16_t len = 0;
    // void *recv_data;
    // recv_data = (void *)pvPortMalloc(TCP_BUF_SIZE);
    // printf("Param->uart_queue = %p  \n*Param->uart_queue = %p\n", Param->uart_queue, *Param->uart_queue);
    QueueHandle_t buff_queue = *Param->tx_buff_queue;
    //tcpip_adapter_ip_info_t ip_info;
    /* Create a new connection identifier. */
    /* Bind connection to well known port number 7. */
    create_tcp_server(Param->port,&conn);
    // while (conn == NULL )
    // {
    //     conn = netconn_new(NETCONN_TCP);
    // }
    // // netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    // //netconn_bind(*conn, &ip_info.ip, Param->port);
    //  /* Bind connection to well known port number 7. */
    // netconn_bind(conn, IP_ADDR_ANY, Param->port);
    // netconn_listen(conn); /* Grab new connection. */
    // netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    //MY_IP_ADDR(&ip_info.ip, TCP_IP_ADDRESS);
   // netconn_bind(conn, &ip_info.ip, Param->port);
   
    /* Tell connection to go into listening mode. */
    //netconn_listen(conn);
    //printf("listening : %p\r\n",conn);
   
    /* Grab new connection. */
    while (1)
    {
        int re_err;
        err = netconn_accept(conn, &newconn);

        /* Process the new connection. */

        if (err == ERR_OK)
        {

            struct netbuf *buf;
            struct netbuf *buf2;
            void *data;
            while (1)
            {
                re_err = (netconn_recv(newconn, &buf));
                if (re_err == ERR_OK)
                {
                    do
                    {
                        events event;
                        netbuf_data(buf, &data, &event.buff_len);
                        event.buff = data;
                        if (xQueueSend(buff_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)                   
                                ESP_LOGE(TCP_TAG, "SEND TO QUEUE FAILD\n");
                    } while ((netbuf_next(buf) >= 0));
                    netbuf_delete(buf2);
                    re_err = (netconn_recv(newconn, &buf2));
                    if (re_err == ERR_OK)
                    {
                        do
                        {
                            events event;
                            netbuf_data(buf2, &data, &event.buff_len);
                            event.buff = data;
                            if (xQueueSend(buff_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)                   
                                ESP_LOGE(TCP_TAG, "SEND TO QUEUE FAILD\n");
                        } while ((netbuf_next(buf2) >= 0));
                        netbuf_delete(buf);
                    }
                }
                else if (re_err == ERR_CLSD)
                {
                    ESP_LOGE(TCP_TAG, "DISCONNECT PORT:%d\n", Param->port);
                    //netbuf_delete(buf);
                    //netbuf_delete(buf2);
                    break;
                }
            }
            netconn_close(newconn);
            netconn_delete(newconn);
            netconn_listen(conn);
        }
    }
    vTaskDelete(NULL);
}
void tcp_rev_server(void *Parameter)
{
    TcpParam *Param = (TcpParam *)Parameter;
    struct netconn *conn = NULL;
    struct netconn *newconn = NULL;
    // err_t re_err = 0;
    QueueHandle_t buff_queue = *Param->rx_buff_queue;
    
    /* Create a new connection identifier. */
    create_tcp_server(Param->port,&conn);
    // while (conn == NULL)
    // {
    //     conn = netconn_new(NETCONN_TCP);
    //     printf("CONN: %p\n", conn);
    //     conn->send_timeout = 0;
    // }
    // tcpip_adapter_ip_info_t ip_info;
    // MY_IP_ADDR(&ip_info.ip, TCP_IP_ADDRESS);
    // // netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    // netconn_bind(conn, &ip_info.ip, Param->port);
    // netconn_bind(conn, IP4_ADDR_ANY, Param->port);
    /* Tell connection to go into listening mode. */
    //netconn_listen(conn);
    //printf("PORT: %d\nLISTENING.....\n", Param->port);
    /* Grab new connection. */
    while (1)
    {
        /* Process the new connection. */
        if (netconn_accept(conn, &newconn) == ERR_OK)
        {
            while (1)
            {
                events event;
                int ret = xQueueReceive(buff_queue, &event, portMAX_DELAY);
                if (ret == pdTRUE)
                {
                    netconn_write(newconn, event.buff, event.buff_len, NETCONN_NOCOPY);
                }
                if (get_connect_state(newconn) == ERR_CLSD)
                {
                    netconn_close(newconn);
                    netconn_delete(newconn);
                    netconn_listen(conn);
                    ESP_LOGE(TCP_TAG, "DISCONNECT PORT:%d\n", Param->port);
                    break;
                }
            }
        }
    }
    vTaskDelete(NULL);
}

void tcp_server_subtask(void* Parameter)
{
    SubTcpParam* Param = (SubTcpParam*)Parameter;
    TcpParam *tcp_param = Param->TcpParam;
    QueueHandle_t rx_buff_queue = *tcp_param->rx_buff_queue;
    printf("tcp_rx_queue rx: %p\n", tcp_param->rx_buff_queue);
    //struct netconn *conn = Param->conn;
    struct netconn *newconn = *(Param->newconn);
    uint8_t task_flag = *(Param->task_flag);
    while (task_flag)
    {
   
        events event;
        int ret = xQueueReceive(rx_buff_queue, &event, portMAX_DELAY);
        if (ret == pdTRUE)
        {
            netconn_write(newconn, event.buff, event.buff_len, NETCONN_NOCOPY);
        }
    }
    vTaskDelete(NULL);
}

TcpTaskHandle_t* TcpTaskCareate(void *Parameter)
{
    TcpParam *Param = (TcpParam *)Parameter;
    const char allname[] = "ALL";
    const char rxname[] = "Rec";
    const char txname[] = "Tran";
    char pcName[18];
    switch (Param->mode)
    {
    case SEND:
        sprintf(pcName, "Tcp%s%d",txname,TcpHandle.TaskNum);
        xTaskCreatePinnedToCore(tcp_send_server, (const char* const)pcName, 5120, Parameter, 14,&TCP_TASK_HANDLE[TcpHandle.TaskNum],0);
        TcpHandle.TaskHandle[TcpHandle.TaskNum] = &TCP_TASK_HANDLE[TcpHandle.TaskNum];
        TcpHandle.TaskNum++;
        break;
    case RECEIVE:
        sprintf(pcName, "Tcp%s%d",rxname,TcpHandle.TaskNum);
        xTaskCreatePinnedToCore(tcp_rev_server, (const char* const)pcName, 5120, Parameter, 14,&TCP_TASK_HANDLE[TcpHandle.TaskNum],0);
        TcpHandle.TaskHandle[TcpHandle.TaskNum] = &TCP_TASK_HANDLE[TcpHandle.TaskNum];
        TcpHandle.TaskNum++;
        break;
    case ALL:
        sprintf(pcName, "Tcp%s%d",allname,TcpHandle.TaskNum);
        xTaskCreatePinnedToCore(tcp_server, (const char* const)pcName, 5120, Parameter, 14,&TCP_TASK_HANDLE[TcpHandle.TaskNum],0);
        TcpHandle.TaskHandle[TcpHandle.TaskNum] = &TCP_TASK_HANDLE[TcpHandle.TaskNum];
        TcpHandle.TaskNum++;
        TcpHandle.TaskHandle[TcpHandle.TaskNum] = &SubTask_Handle;
        TcpHandle.TaskNum++;
        break;
    default:
        break;
    }
return &TcpHandle;
}

uint8_t TcpTaskAllDelete(TcpTaskHandle_t* TcpHandle)
{
    for (int i = 0; i < TcpHandle->TaskNum; i++)
    {
        if ((TcpHandle->TaskHandle[i]) != NULL)
        {
            vTaskDelete((TcpHandle->TaskHandle[i]));
        }
        else
        {
            break;
        }
    }
    TcpHandle->TaskNum = 0;
    return ESP_OK;
}
void tcp_server(void *Parameter)
{
    TcpParam *Param = (TcpParam *)Parameter;
    struct netconn *conn = NULL;
    struct netconn *newconn = NULL;
    uint8_t subtask_flag = 0;
    err_t err = 1;
    QueueHandle_t tx_buff_queue = *Param->tx_buff_queue;
    printf("tcp_tx_queue tx: %p\n", Param->tx_buff_queue);
    /* Create a new connection identifier. */
     create_tcp_server(Param->port,&conn);
    SubTcpParam SubParam = {
        .TcpParam = Param,
        .conn = &conn,
        .newconn = &newconn,
        .task_flag = &subtask_flag,
    };
    /* Tell connection to go into listening mode. */
    //netconn_listen(conn);
    // printf("PORT: %d\nLISTENING.....\n",  Param->port);
    while (1)
    {
         int re_err;
        err = netconn_accept(conn, &newconn);

        /* Process the new connection. */

        if (err == ERR_OK)
        {

            if(!subtask_flag)
            {
                /*Create Receive Subtask*/
                subtask_flag = 1;
                SubParam.newconn = &newconn;
                if (!subtask_one_flag)
                {
                     xTaskCreate(tcp_server_subtask, "tcp_subtask", 4096, (void*)(&SubParam), 14, &SubTask_Handle);
                    subtask_one_flag = 1;
                }
                else
                {
                    xTaskCreate(tcp_server_subtask, "tcp_subtask_1", 4096, (void*)(&SubParam), 14, &SubTask_Handle_1);
                }
                
            }
            /*Create send buffer*/
            struct netbuf *buf;
            struct netbuf *buf2;
            void *data;
            while (1)
            {
                re_err = (netconn_recv(newconn, &buf));
                if (re_err == ERR_OK)
                {
                    do
                    {
                        events tx_event_1;
                        netbuf_data(buf, &data, &tx_event_1.buff_len);
                        tx_event_1.buff = data;
                        //tx_event_1.buff[tx_event_1.buff_len] = '\0';
                       // printf("tcp ： send buff:%s \n", tx_event_1.buff);
                        if (xQueueSend(tx_buff_queue, &tx_event_1, pdMS_TO_TICKS(10)) == pdPASS)                   
                            ;
                        else
                            ESP_LOGE(TCP_TAG, "SEND TO QUEUE FAILD\n");
                    } while ((netbuf_next(buf) >= 0));
                    netbuf_delete(buf2);
                    re_err = (netconn_recv(newconn, &buf2));
                    if (re_err == ERR_OK)
                    {
                        do
                        {
                            events tx_event_2;
                            netbuf_data(buf2, &data, &tx_event_2.buff_len);
                            tx_event_2.buff = data;
                            if (xQueueSend(tx_buff_queue, &tx_event_2, pdMS_TO_TICKS(10)) == pdPASS)                   
                                   ;
                            else
                                ESP_LOGE(TCP_TAG, "SEND TO QUEUE FAILD\n");
                        } while ((netbuf_next(buf2) >= 0));
                        netbuf_delete(buf);
                    }
                }
                else if (re_err == ERR_CLSD)
                {
                    ESP_LOGE(TCP_TAG, "DISCONNECT PORT:%d\n", Param->port);
                    //netconn_close(newconn);
                    // netbuf_delete(buf);
                    // netbuf_delete(buf2);
                    //break;
                }
            }
        
            vTaskDelete(SubTask_Handle);
            subtask_flag = 0;
            netconn_close(newconn);
            netconn_delete(newconn);
            netconn_listen(conn);
        }
    }
}
static err_t get_connect_state(struct netconn *conn)
{
    void *msg;
    err_t err;
    if (sys_arch_mbox_tryfetch(&conn->recvmbox, &msg) != SYS_MBOX_EMPTY)
    {
        lwip_netconn_is_err_msg(msg, &err);
        if (err == ERR_CLSD)
        {
            return ERR_CLSD;
        }
    }
    return ERR_OK;
}