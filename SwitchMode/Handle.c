#include "Handle.h"
#include "InstructionServer/wifi_configuration.h"

#include "UART/uart_analysis_parameters.h"
#include "UART/uart_config.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "TCP-CH/tcp.h"
TcpParam tcp_param;
bool uart_handle_flag = false;
extern uart_init_t c1;
extern uart_init_t c2;
extern bool c1UartConfigFlag;
extern bool c2UartConfigFlag;
extern const char kHeartRet[5]; // 心跳包发送
extern uint8_t uart_flag;
TcpTaskHandle_t *tcphand;
TcpTaskHandle_t *tcphand1;
extern const int TcpHandle_FatherTask_current;
extern TcpTaskHandle_t TCP_TASK_HANDLE[10];
//TcpTaskHandle_t *tcphand;
//TcpTaskHandle_t *tcphand1;
QueueHandle_t uart_queue1 = NULL;
QueueHandle_t uart_queue = NULL;
QueueHandle_t uart_queue2 = NULL;
QueueHandle_t uart_queue3 = NULL;


void DAP_Handle(void) {}
void UART_Handle(void) {
    printf("\nuart_handle_flag\n");
    uart_handle_flag = true;
}
void ADC_Handle(void) {}
void DAC_Handle(void) {}
void PWM_Collect_Handle(void) {}
void PWM_Simulation_Handle(void) {}
void I2C_Handle(void) {}
void SPI_Handle(void) {}
void CAN_Handle(void) {}

void uart_task(int ksock) {
    int written = 0;

    static QueueHandle_t uart_queue1 = NULL;
    static QueueHandle_t uart_queue = NULL;
    static QueueHandle_t uart_queue2 = NULL;
    static QueueHandle_t uart_queue3 = NULL;
    uart_queue = xQueueCreate(10, sizeof(events));
    uart_queue1 = xQueueCreate(50, sizeof(events));

    c1.rx_buff_queue = &uart_queue;
    c1.tx_buff_queue = &uart_queue1;
    printf("c1 rx_buff_queue:%p\n",(c1.rx_buff_queue));
    printf("c1 tx_buff_queue:%p\n",(c1.tx_buff_queue));

    printf("uart_queue rx: %p  uart_queue1 tx: %p\n", &uart_queue, &uart_queue1);

    // xTaskCreatePinnedToCore(uart_rev, "uartr", 5120, (void *)&c1, 10, &xHandle, 0);
    if (c1UartConfigFlag == true) {
        Create_Uart_Task(&c1);
        if (uart_flag == 1) {
            TcpTaskAllDelete(TCP_TASK_HANDLE);
             static TcpParam tp0 =
                {
                    .rx_buff_queue = &uart_queue,
                    .tx_buff_queue = &uart_queue1,
                    .mode = ALL,
                    .port = CH2,
                };
            tcphand = TcpTaskCreate(&tp0);
        }else if(uart_flag == 0){
            static TcpParam tp0 =
                {
                    .rx_buff_queue = &uart_queue,
                    .tx_buff_queue = &uart_queue1,
                    .mode = ALL,
                    .port = CH2,
                };
            printf("tp0 rx_buff_queue:%p\n",tp0.rx_buff_queue);
            printf("tp0 tx_buff_queue:%p\n",tp0.tx_buff_queue);
            tcphand = TcpTaskCreate(&tp0);
        }
        c1UartConfigFlag = false;
    }

//    if (c2UartConfigFlag == true ) {
//        Create_Uart_Task(&c2);
//           static TcpParam tp2 =
//            {
//                .rx_buff_queue = &uart_queue2,
//                .tx_buff_queue = &uart_queue3,
//                .mode = ALL,
//                .port = CH3,
//            };
//        tcphand1 = TcpTaskCreate(&tp2);
//        c2UartConfigFlag = false;
//    } else if (c2UartConfigFlag == true && uart_flag == 1) {
//        Create_Uart_Task(&c2);
//           static TcpParam tp2 =
//            {
//                .rx_buff_queue = &uart_queue2,
//                .tx_buff_queue = &uart_queue3,
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
    // xTaskCreatePinnedToCore(uart_send, "uartt", 5120, (void *)&c2, 10, &xHandle, 1);
    // printf("create uartt \n");
    do {
        written = send(ksock, "OK!\r\n", 5, 0);
    } while (written <= 0);
}