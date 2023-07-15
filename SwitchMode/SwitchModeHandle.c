#include "SwitchModeHandle.h"


#include "UART/uart_analysis_parameters.h"
#include "UART/uart_config.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <esp_log.h>

#include "TCP-CH/tcp.h"
#include "WirelessDAP_main/DAP_handle.h"
#include "WirelessDAP_main/dap_tcp_server.h"
TcpParam tcp_param;
enum WorkMode working_mode = NONE_MODE;
extern UartInitT c1;
extern UartInitT c2;
extern bool c1UartConfigFlag;
extern bool c2UartConfigFlag;
extern const char kHeartRet[5]; // 心跳包发送
extern uint8_t uart_flag;
TcpTaskHandleT *tcphand;
TcpTaskHandleT *tcphand1;
extern const int kTcpHandleFatherTaskCurrent;
extern TcpTaskHandleT TCP_TASK_HANDLE[10];
//TcpTaskHandleT *tcphand;
//TcpTaskHandleT *tcphand1;
QueueHandle_t uart_queue1 = NULL;
QueueHandle_t uart_queue = NULL;
QueueHandle_t uart_queue2 = NULL;
QueueHandle_t uart_queue3 = NULL;
static const char *TAG = "ModeSwitcher";
TaskHandle_t kDAPTaskHandle = NULL;
void USBIP_server_task(void *pvParameters);
void DAP_Thread(void *argument);
void DAPHandle(void) {
    ESP_LOGI(TAG, "Enter DAP Mode");
    working_mode = DAP;
    // USBIP的TCP服务器
    xTaskCreatePinnedToCore(USBIP_server_task, "USBIP_server_task", 4096, NULL, 14, NULL, 1);

    // DAP调试器的服务器
    xTaskCreatePinnedToCore(DAP_Thread, "DAP_Task", 2048, NULL, 10, &kDAPTaskHandle, 2);
}
void UartHandle(void) {
    ESP_LOGI(TAG, "Enter Uart Mode");
    working_mode = UART;
}
void ADCHandle(void) {}
void DACHandle(void) {}
void PwmCollectHandle(void) {}
void PwmSimulationHandle(void) {}
void I2CHandle(void) {}
void SpiHandle(void) {}
void CanHandle(void) {}

void UartTask(int ksock) {
    int written = 0;

    static QueueHandle_t uart1_tx_queue = NULL;
    static QueueHandle_t uart1_rx_queue = NULL;
    static QueueHandle_t uart2_rx_queue = NULL;
    static QueueHandle_t uart2_tx_queue = NULL;
    uart1_rx_queue = xQueueCreate(10, sizeof(events));
    uart1_tx_queue = xQueueCreate(50, sizeof(events));

    c1.rx_buff_queue_ = &uart1_rx_queue;
    c1.tx_buff_queue_ = &uart1_tx_queue;
    printf("c1 rx_buff_queue_:%p\n",(c1.rx_buff_queue_));
    printf("c1 tx_buff_queue_:%p\n",(c1.tx_buff_queue_));

    printf("uart1_rx_queue rx: %p  uart1_tx_queue tx: %p\n", &uart1_rx_queue, &uart1_tx_queue);

    // xTaskCreatePinnedToCore(UartRev, "uartr", 5120, (void *)&c1, 10, &xHandle, 0);
    if (c1UartConfigFlag == true)
    {

        if (uart_flag == 1)
        {//串口已被占用，重新配置
            UartSetup(&c1);
            TcpTaskAllDelete(TCP_TASK_HANDLE);
             static TcpParam tp0 =
                {
                    .rx_buff_queue_ = &uart1_rx_queue,
                    .tx_buff_queue_ = &uart1_tx_queue,
                    .mode = TCP_ALL,
                    .port = CH2,
                };
            tcphand = TcpTaskCreate(&tp0);
        }
        else if(uart_flag == 0)
        {
            CreateUartTask(&c1);
            static TcpParam tp0 =
                {
                    .rx_buff_queue_ = &uart1_rx_queue,
                    .tx_buff_queue_ = &uart1_tx_queue,
                    .mode = TCP_ALL,
                    .port = CH2,
                };

            tcphand = TcpTaskCreate(&tp0);
        }
        c1UartConfigFlag = false;//0
    }

//    if (c2UartConfigFlag == true ) {
//        CreateUartTask(&c2);
//           static tcp_param_ tp2 =
//            {
//                .rx_buff_queue_ = &uart2_rx_queue,
//                .tx_buff_queue_ = &uart2_tx_queue,
//                .mode = ALL,
//                .port = CH3,
//            };
//        tcphand1 = TcpTaskCreate(&tp2);
//        c2UartConfigFlag = false;
//    } else if (c2UartConfigFlag == true && uart_flag == 1) {
//        CreateUartTask(&c2);
//           static tcp_param_ tp2 =
//            {
//                .rx_buff_queue_ = &uart2_rx_queue,
//                .tx_buff_queue_ = &uart2_tx_queue,
//                .mode = ALL,
//                .port = CH3,
//            };
//        tcphand1 = TcpTaskCreate(&tp2);
//        c2UartConfigFlag = false;
//    }

    // do{
    //     written=send(ksock, kHeartRet, 5, 0);
    //     printf("%d\n",written);
    // }while(written<=0);
    // printf("create uartr \n");
    // xTaskCreatePinnedToCore(UartSend, "uartt", 5120, (void *)&c2, 10, &xHandle, 1);
    // printf("create uartt \n");
    do {
        written = send(ksock, "OK!\r\n", 5, 0);
    } while (written <= 0);
}