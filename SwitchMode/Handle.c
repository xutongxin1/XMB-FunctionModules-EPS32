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
extern const char kHeartRet[5]; //心跳包发送

void DAP_Handle(void){}
void UART_Handle(void){
    printf("\nuart_handle_flag\n");
    uart_handle_flag=true;
}
void ADC_Handle(void){}
void DAC_Handle(void){}
void PWM_Collect_Handle(void){}
void PWM_Simulation_Handle(void){}
void I2C_Handle(void){}
void SPI_Handle(void){}
void CAN_Handle(void){}


void uart_task(int ksock)
{
    int written=0;
    TcpTaskHandle_t* tcphand;    
    static QueueHandle_t uart_queue1 = NULL;
    static QueueHandle_t uart_queue = NULL;
    uart_queue = xQueueCreate(10, sizeof(events));
    uart_queue1 = xQueueCreate(50, sizeof(events));
    c1.rx_buff_queue = &uart_queue;
    c1.tx_buff_queue = &uart_queue1;
    c2.tx_buff_queue = &uart_queue1;
    c2.rx_buff_queue = &uart_queue;
    printf("uart_queue rx: %p  uart_queue1 tx: %p\n", &uart_queue, &uart_queue1);
    TaskHandle_t xHandle = NULL;

    //xTaskCreatePinnedToCore(uart_rev, "uartr", 5120, (void *)&c1, 10, &xHandle, 0);
    if(c1UartConfigFlag==true){
        Create_Uart_Task((void *)&c1);
        static TcpParam tp = 
        {
        .rx_buff_queue = &uart_queue,
        .tx_buff_queue = &uart_queue1,
        .mode = ALL,
        .port = CH2,

        };
        tcphand = TcpTaskCareate((void*) &tp);
        c1UartConfigFlag=false;
    }    
    if(c2UartConfigFlag==true){
        Create_Uart_Task((void *)&c2);
        static TcpParam tp1 = 
        {
        .rx_buff_queue = &uart_queue,
        .tx_buff_queue = &uart_queue1,
        .mode = ALL,
        .port = CH3,

        };
        tcphand = TcpTaskCareate((void*) &tp1);
        c2UartConfigFlag=false;
    }
    // do{
    //     written=send(ksock, kHeartRet, 5, 0);
    //     printf("%d\n",written);
    // }while(written<=0);
    // printf("create uartr \n");
    // xTaskCreatePinnedToCore(uart_send, "uartt", 5120, (void *)&c2, 10, &xHandle, 1);
    // printf("create uartt \n");

    do{
        written = send(ksock, "OK!\r\n", 5, 0);
    }while(written<=0);



}