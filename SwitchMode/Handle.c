#include "Handle.h"
#include "InstructionServer/wifi_configuration.h"

#include "UART/uart_analysis_parameters.h"
#include "UART/uart_config.h"

#include "TCP-CH/tcp.h"

bool uart_handle_flag = false;

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
    c1.buff_queue = &uart_queue;
    c3.buff_queue = &uart_queue1;
    // static struct TcpParam tcp_param = {
    //     .mode = RECEIVE,
    //     .ch = CH1,
    // };
    tcp_param.mode = RECEIVE;
    tcp_param.ch = CH1;
    tcp_param.buff_queue = &uart_queue;
    static  TcpParam tcp_param1 = {
        .mode = SEND,
        .ch = CH3,
    };
    tcp_param1.buff_queue = &uart_queue1;
    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore(uart_rev, "uartr", 5120, (void *)&c1, 10, &xHandle, 0);
    printf("create uartr \n");
    xTaskCreatePinnedToCore(uart_send, "uartt", 5120, (void *)&c3, 10, &xHandle, 0);
    printf("create uartt \n");

    xTaskCreatePinnedToCore(tcp_rev_server, "tcpr", 5120, (void *)&tcp_param, 10, NULL, 1);
    printf("create tcpr \n");
    xTaskCreatePinnedToCore(tcp_send_server, "tcpt", 5120, (void *)&tcp_param1, 10, NULL, 1);
    printf("create tcpt \n");

}