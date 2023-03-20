#include "UART/uart_analysis_parameters.h"
#include "cJSON.h"
uart_init_t c1;
uart_init_t c2;
uart_init_t c3;

bool c1UartConfigFlag = false; 
bool c2UartConfigFlag = false;

int uart_1_parameter_analysis(void *attach_rx_buffer,uart_init_t* uartconfig) {

    cJSON *pu1 = cJSON_GetObjectItem(attach_rx_buffer, "u1"); // 解析c1字段内容
    printf("\nu1:\n");
         //是否指令为空
         if (pu1 != NULL)
         {
            cJSON * item;

            uartconfig->pin.rx_pin=34;

            //uartconfig->pin.MODE = RX;

            uartconfig->uart_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;

            uartconfig->uart_num = UART_NUM_2;

            item=cJSON_GetObjectItem(pu1,"mode");
            uartconfig->mode = item->valueint;
            printf("mode = %d\n",uartconfig->mode);

            item=cJSON_GetObjectItem(pu1,"band");
            uartconfig->uart_config.baud_rate = item->valueint;
            printf("band = %d\n",uartconfig->uart_config.baud_rate);

            item=cJSON_GetObjectItem(pu1,"parity");
            uartconfig->uart_config.parity = item->valueint;
            printf("parity = %d\n",uartconfig->uart_config.parity);

            item=cJSON_GetObjectItem(pu1,"data");
            uartconfig->uart_config.data_bits = (item->valueint)-5;
            printf("data = %d\n",uartconfig->uart_config.data_bits);

            item=cJSON_GetObjectItem(pu1,"stop");
            uartconfig->uart_config.stop_bits=item->valueint;
            printf("stop = %d\n",uartconfig->uart_config.stop_bits);

            return 1;
         }
     
    return 0;
}



int uart_2_parameter_analysis(void *attach_rx_buffer,uart_init_t* uartconfig) {
    //首先整体判断是否为一个json格式的数据
    cJSON *pu2 = cJSON_GetObjectItem(attach_rx_buffer, "u2"); // 解析c1字段内容
    printf("\nu2:\n");
        //是否指令为空
        if (pu2 != NULL)
        {
            cJSON * item;

            item=cJSON_GetObjectItem(pu2,"mode");
            uartconfig->mode = item->valueint;
            printf("mode = %d\n",uartconfig->mode);            

            uartconfig->uart_num = UART_NUM_1;

            item=cJSON_GetObjectItem(pu2,"band");
            uartconfig->uart_config.baud_rate = item->valueint;
            printf("band = %d\n",uartconfig->uart_config.baud_rate);
            
            item=cJSON_GetObjectItem(pu2,"parity");
            uartconfig->uart_config.parity = item->valueint;
            printf("parity = %d\n",uartconfig->uart_config.parity);
            
            item=cJSON_GetObjectItem(pu2,"data");
            uartconfig->uart_config.data_bits = (item->valueint)-5;
            printf("data = %d\n",uartconfig->uart_config.data_bits);
            
            item=cJSON_GetObjectItem(pu2,"stop");
            uartconfig->uart_config.stop_bits = item->valueint;
            printf("stop = %d\n",uartconfig->uart_config.stop_bits);
            
            uartconfig->pin.tx_pin=25;

            uartconfig->uart_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;

            return 1;
        }
    return 0;
}

