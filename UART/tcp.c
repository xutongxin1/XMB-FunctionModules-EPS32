
#include "sdkconfig.h"

#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdatomic.h>

#include "main/wifi_configuration.h"
#include "main/bps_config.h"
#include "main/tcp.h"

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
#include <lwip/netdb.h>

#define EVENTS_QUEUE_SIZE 10

#ifdef CALLBACK_DEBUG
#define debug(s, ...) os_printf("%s: " s "\n", "Cb:", ##__VA_ARGS__)
#else
#define debug(s, ...)
#endif
#define MY_IP_ADDR(...) IP4_ADDR(__VA_ARGS__)
union lwip_sock_lastdata
{
    struct netbuf *netbuf;
    struct pbuf *pbuf;
};

static uint16_t choose_port(uint8_t pin)
{
    switch (pin)
    {
    case 25:
        return 1922;
    case 26:
        return 1923;
    case 34:
        return 1921;
    default:
        return -1;
    }
}

void tcp_send_server(void *Parameter)
{
    os_printf("Parameter = %p  \n", Parameter);
    struct TcpUartParam *Param = (struct TcpUartParam *)Parameter;
    os_printf("Param = %p  \n", Param);
    struct netconn *conn = NULL;
    os_printf("conn\n");
    struct netconn *newconn = NULL;
    
    err_t err = 1;
    uint16_t len = 0;
    struct netbuf data;
    struct netbuf* tcpdata = &data;
    os_printf("tcpdata\n");
    char buffer[UART_BUF_SIZE] = { 0 }; //缓冲区
    os_printf("Param->uart_queue = %p  \n*Param->uart_queue = %p\n", Param->uart_queue, *Param->uart_queue);
    QueueHandle_t uart_queue = *Param->uart_queue;
    uart_events event;
    tcpip_adapter_ip_info_t ip_info;
    /* Create a new connection identifier. */
    /* Bind connection to well known port number 7. */
    while (conn == NULL)
    {
        conn = netconn_new(NETCONN_TCP);
        os_printf("CONN: %p\n", conn);
    }
    netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    MY_IP_ADDR(&ip_info.ip, TCP_IP_ADDRESS);
    netconn_bind(conn, &ip_info.ip, choose_port(Param->ch));

    /* Tell connection to go into listening mode. */
    netconn_listen(conn);
    /* Grab new connection. */
    while (1)
    {
        for (; err != ERR_OK;)
        {
            err = netconn_accept(conn, &newconn);
            vTaskDelay(10);
        }
        os_printf("accepted new connection %p\n", newconn);
        /* Process the new connection. */

        if (err == ERR_OK)
        {
            while (1)
            {
                while ((netconn_recv(newconn, &tcpdata)) == ERR_OK)
                {
                    do
                    {
                        netbuf_data(tcpdata, (void *)buffer, &len);
                        strncpy(event.uart_buffer, buffer, len);
                        xQueueSend(uart_queue, &event, pdMS_TO_TICKS(10));
                    } while (netbuf_next(tcpdata) >= 0);
                }
            }
        }
    }
    vTaskDelete(NULL);
}
void tcp_rev_server(void *Parameter)
{
    struct TcpUartParam *Param = (struct TcpUartParam *)Parameter;
    struct netconn *conn = NULL, *newconn;
    err_t err = 1;
    uint16_t len = 0;
    uart_events event;
    QueueHandle_t uart_queue = *(Param->uart_queue);
    tcpip_adapter_ip_info_t ip_info;
    /* Create a new connection identifier. */
    /* Bind connection to well known port number 7. */
    while (conn == NULL)
    {
        conn = netconn_new(NETCONN_TCP);
        os_printf("CONN: %p\n", conn);
        conn->send_timeout = 0;
        vTaskDelay(100);
    }
    MY_IP_ADDR(&ip_info.ip, TCP_IP_ADDRESS);
    netconn_set_nonblocking(conn, NETCONN_FLAG_NON_BLOCKING);
    netconn_bind(conn, &ip_info.ip, choose_port(Param->ch));
    /* Tell connection to go into listening mode. */
    netconn_listen(conn);
    os_printf("PORT: %dLISTENING.....\n",choose_port(Param->ch));
    /* Grab new connection. */
    while (1)
    {
        for (; err != ERR_OK;)
        {
            err = netconn_accept(conn, &newconn);
        }
        os_printf("accepted new connection %p\n", newconn);
        /* Process the new connection. */
        netconn_write(newconn,"ACCEPTED", 8, NETCONN_COPY);
            while (1)
            {
                os_printf("wait message\n");
                int ret = xQueueReceive(uart_queue, &event, portMAX_DELAY);
                
                if (ret == pdTRUE)
                {
                    os_printf("event.uart_buffer =  %s\n len = %d\n", event.uart_buffer, event.buff_len);
                    netconn_write(newconn, event.uart_buffer, event.buff_len, NETCONN_COPY);
                }
            }
    }
    vTaskDelete(NULL);
}