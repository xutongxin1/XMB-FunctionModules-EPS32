#include "my_netconn.h"
#include "lwip/opt.h"
#include "UART/uart_val.h"
#include "UART/uart_config.h"
#include "driver/uart.h"
#if LWIP_NETCONN
#include "lwip/sys.h"
#include "lwip/api.h"
#define TCPECHO_THREAD_PRIO(osPriorityAboveNormal)
static void mytcp_thread(void *arg)
{
    struct netconn *conn, *newconn;
    err_t err;
    u8_t tx_buf[20]; // 发送缓冲区
    u16_t rx_len = 0;

    u8_t *temp = "cnt:%d\r\n";
    u16_t cnt = 0; // 要通过TCP显示的计数值
    LWIP_UNUSED_ARG(arg);
    /* Create a new connection identifier. */
    /* Bind connection to well known port number 7. */
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, 7);
    LWIP_ERROR("my tcpdemo: invalid conn", (conn != NULL), return;);
    /* Tell connection to go into listening mode. */
    netconn_listen(conn); /* Grab new connection. */
    err = netconn_accept(conn, &newconn);
    printf("accepted new connection %p\n", newconn);
    /* Process the new connection. */
    if (err == ERR_OK)
    {
        while (1)
        {
            sprintf(tx_buf, temp, cnt);
            tx_len = strlen((const char *)tx_buf);
            cnt++;
            err = netconn_write(newconn, tx_buf, tx_len, NETCONN_COPY);
            vTaskDelay(1000);
        }
    }
}
// 创建tcp线程
void mytcp_init(void)
{
    sys_thread_new("tcpecho_thread", mytcp_thread, NULL, (configMINIMAL_STACK_SIZE * 2), TCPECHO_THREAD_PRIO);
}

#endif 