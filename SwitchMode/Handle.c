#include "Handle.h"
#include "InstructionServer/wifi_configuration.h"

#include "UART/uart_analysis_parameters.h"
#include "UART/uart_config.h"

#include "TCP-CH/tcp.h"
TcpParam tcp_param;
bool uart_handle_flag = false;
extern uart_init_t c1;
extern uart_init_t c2;

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


void uart_task(void)
{
    static QueueHandle_t uart_queue1 = NULL;
    static QueueHandle_t uart_queue = NULL;
    uart_queue = xQueueCreate(10, sizeof(events));
    uart_queue1 = xQueueCreate(50, sizeof(events));
    c1.rx_buff_queue = &uart_queue;
    c2.tx_buff_queue = &uart_queue1;
    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore(uart_rev, "uartr", 5120, (void *)&c1, 10, &xHandle, 0);
    printf("create uartr \n");
    xTaskCreatePinnedToCore(uart_send, "uartt", 5120, (void *)&c2, 10, &xHandle, 1);
    printf("create uartt \n");


    TcpTaskHandle_t* tcphand;
    static TcpParam tp = 
    {
        .rx_buff_queue = &uart_queue,
        .tx_buff_queue = &uart_queue1,
        .mode = SEND,
        .port = CH2,

    };
    tcphand = TcpTaskCareate((void*) &tp);
    tp.mode = RECEIVE;
    tp.port = CH3;
    tcphand = TcpTaskCareate((void*) &tp);
    tp.mode = ALL;
    tp.port = CH4;
    tcphand = TcpTaskCareate((void*) &tp);

    



}