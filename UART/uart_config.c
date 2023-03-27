#include "sdkconfig.h"

#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdatomic.h>

#include "InstructionServer/wifi_configuration.h"

#include "UART/uart_val.h"
#include "UART/uart_config.h"

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

const char *UART_TAG = "UART";

static TaskHandle_t uart_task_handle[4];

static uart_manage_t uart_manage;
// void uart_rev(void *uartParameter)
// {
//     uart_configrantion config = *(uart_configrantion *)uartParameter;
//     uart_port_t uart_num = config.uart_num;
//     uart_setup(&config);
//     char buffer[UART_BUF_SIZE];
//     int uart_buf_len = 0;
//     events event;
//     QueueHandle_t uart_queue = *config.buff_queue;
//     while (1)
//     {
//         uart_get_buffered_data_len(uart_num, (size_t *)&uart_buf_len);
//         if (uart_buf_len)
//         {
//             uart_buf_len = uart_buf_len > UART_BUF_SIZE ? UART_BUF_SIZE : uart_buf_len;
//             uart_buf_len = uart_read_bytes(uart_num, buffer, uart_buf_len, pdMS_TO_TICKS(5));
//             buffer[uart_buf_len] = '\0';
//             ESP_LOGE(UART_TAG, "buffer = %s  \nuart_buf_len = %d\n", buffer, uart_buf_len);
//             if (uart_buf_len != 0)
//             {
//                 strncpy(event.buff, buffer, uart_buf_len);
//                 ESP_LOGE(UART_TAG, "event buffer = %s  \n", event.buff);
//                 event.buff_len = uart_buf_len;
//                 uart_buf_len = 0;
//                 if (xQueueSend(uart_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)
//                 {
//                     ESP_LOGE(UART_TAG, "SEND TO QUEUE\n");
//                 }
//                 else
//                 {
//                     ESP_LOGE(UART_TAG, "SEND TO QUEUE FAILD\n");
//                 }
//             }
//         }
//     }
//     vTaskDelete(NULL);
// }

// void uart_send(void *uartParameter)
// {
//     uart_configrantion config = *(uart_configrantion *)uartParameter;
//     uart_port_t uart_num = config.uart_num;
//     uart_setup(&config);
//     // ESP_LOGE(UART_TAG, "config.uart_queue = %p  \n*config.uart_queue = %p\n", config.buff_queue, *config.buff_queue);
//     QueueHandle_t uart_queue = *config.buff_queue;
//     while (1)
//     {
//         events event;
//         // ESP_LOGE(UART_TAG, "&event: %p", &event);
//         while (xQueueReceive(uart_queue, &event, pdMS_TO_TICKS(100)) != pdTRUE)
//             ;
//         if (event.buff_len != 0)
//         {
//             uart_write_bytes(uart_num, (const char *)event.buff, event.buff_len);
//         }
//     }
//     vTaskDelete(NULL);
// }

// void uart_setup(uart_init_t *config)
// {
//     ESP_LOGE(UART_TAG, "pin.ch = %d  pin.mode = %d,\n", config->pin.CH, config->pin.MODE);
//     if (config->pin.MODE == TX)
//     {
//         uart_set_pin(config->uart_num, config->pin.CH, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//         ESP_LOGE(UART_TAG, "config->uart_num = %d\n", config->uart_num);
//         uart_param_config(config->uart_num, &config->uart_config);
//         ESP_ERROR_CHECK(uart_driver_install(config->uart_num, 129, UART_BUF_SIZE, 0, NULL, 0));
//         ESP_LOGE(UART_TAG, "uart bridge init successfully\n");
//     }
//     else
//     {
//         uart_set_pin(config->uart_num, UART_PIN_NO_CHANGE, config->pin.CH, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//         ESP_LOGE(UART_TAG, "config->uart_num = %d\n", config->uart_num);
//         uart_param_config(config->uart_num, &config->uart_config);
//         ESP_ERROR_CHECK(uart_driver_install(config->uart_num, UART_BUF_SIZE, 0, 0, NULL, 0));
//         ESP_LOGE(UART_TAG, "uart bridge init successfully\n");
//     }
// }

uint8_t get_uart_manage_id(uart_port_t uart_num)
{
     if (uart_num == UART_NUM_0)
    {
        return -1;
    }
    for (int i = 0; i < uart_manage.existed_num; i++)
    {
        if(uart_manage.existed_port[i] == uart_num)
        {
            return i;
        }
    }
    return 0;    
}
uart_err_t uart_state_register(uart_init_t *config)
{
    uart_init_t *param = config;
    uart_port_t uart_num = param->uart_num;
    uint8_t id = uart_manage.existed_num;
    uart_manage.existed_num++;
    if (id == 2)
    {
       return UART_ERROR;
    }
    uart_manage.task_handle[id].port = uart_num;
    uart_manage.state[id].rx_pin = param->pin.rx_pin;
    uart_manage.state[id].tx_pin = param->pin.tx_pin;
    uart_manage.state[id].baud_rate = param->uart_config.baud_rate;
    uart_manage.state[id].data_bit = param->uart_config.data_bits;
    uart_manage.state[id].parity_bit = param->uart_config.parity;
    uart_manage.state[id].stop_bit = param->uart_config.stop_bits;
    return UART_OK;
}

uart_port_t get_uart_free_num()
{
    if(is_uart_num_free(UART_NUM_1) == UART_OK)
    {
        return UART_NUM_1;
    }
    if(is_uart_num_free(UART_NUM_2) == UART_OK)
    {
        return UART_NUM_2;
    }
    return UART_NUM_0;
}

uart_err_t is_uart_num_free(uart_port_t uart_num)
{
    if (uart_num == UART_NUM_0)
    {
        return UART_NUM_EXISTED;
    }
    for (int i = 0; i < uart_manage.existed_num; i++)
    {
        if(uart_manage.existed_port[i] == uart_num)
        {
            return UART_NUM_EXISTED;
        }
    }
    return UART_OK;
}

uart_err_t uart_setup(uart_init_t *config)
{     
    config->uart_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;
    config->uart_config.source_clk  = UART_SCLK_APB;
    config->uart_num = get_uart_free_num();
    if (is_uart_num_free(config->uart_num))
    {
         ESP_LOGE(UART_TAG, "uart NUM existed\r\n");
        return UART_NUM_EXISTED;
    }
    if (uart_state_register(config))
    {
         ESP_LOGE(UART_TAG, "uart register fail\r\n");
        return UART_REGISTER_FAIL;
    }
    if(uart_set_pin(config->uart_num, config->pin.tx_pin, config->pin.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE))
    {
        ESP_LOGE(UART_TAG, "uart set pin fail\r\n");
        return UART_SET_PAIN_FAIL;
    }
    if(uart_param_config(config->uart_num, &config->uart_config))
    {
        ESP_LOGE(UART_TAG, "uart config fail\n");
        return UART_CONFIG_FAIL;
    }
    if(uart_driver_install(config->uart_num, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0))
    {
        ESP_LOGE(UART_TAG, "uart init fail\n");
        return UART_INSTALL_FAIL;
    }
    return UART_OK;
}

uart_err_t Create_Uart_Task(void *Parameter)
{
    uart_init_t* uart_config = (uart_init_t*)Parameter;
    if(uart_setup(uart_config))
    {
        return uart_setup(uart_config);
    }
    uint8_t id = get_uart_manage_id(uart_config->uart_num);
    const char allname[] = "ALL";
    const char rxname[] = "Rec";
    const char txname[] = "Tran";
    char pcName[18];
    switch (uart_config->mode)
    {
    case Send:
        sprintf(pcName, "Uart%s%d",txname,uart_manage.task_num);
        xTaskCreate(uart_send, (const char* const)pcName, 5120, Parameter, 14,&uart_task_handle[uart_manage.task_num]);
        uart_manage.task_num++;
        uart_manage.task_handle[id].handle[uart_manage.task_handle[id].task_num] = &uart_task_handle[uart_manage.task_num];
        uart_manage.task_handle[id].task_num++;
        break;
    case Receive:
        sprintf(pcName, "Uart%s%d",rxname,uart_manage.task_num);
        xTaskCreate(uart_rev, (const char* const)pcName, 5120, Parameter, 14,&uart_task_handle[uart_manage.task_num]);
        uart_manage.task_num++;
        uart_manage.task_handle[id].handle[uart_manage.task_handle[id].task_num] = &uart_task_handle[uart_manage.task_handle[id].task_num];
        uart_manage.task_handle[id].task_num++;
        break;
    case All:
        sprintf(pcName, "Uart%s%s%d",allname,txname,uart_manage.task_num);
        xTaskCreate(uart_send, (const char* const)pcName, 5120, Parameter, 14,&uart_task_handle[uart_manage.task_num]);
        uart_manage.task_num++;
        uart_manage.task_handle[id].handle[uart_manage.task_handle[id].task_num] = &uart_task_handle[uart_manage.task_num];
        uart_manage.task_handle[id].task_num++;
        sprintf(pcName, "Uart%s%s%d",allname,rxname,uart_manage.task_num);
        xTaskCreate(uart_rev, (const char* const)pcName, 5120, Parameter, 14,&uart_task_handle[uart_manage.task_num]);
        uart_manage.task_num++;
        uart_manage.task_handle[id].handle[uart_manage.task_handle[id].task_num] = &uart_task_handle[uart_manage.task_num];
        uart_manage.task_handle[id].task_num++;
        break;
    default:
        break;
    }
    return UART_OK;
}

uart_err_t Delete_All_Uart_Task()
{
    for (int i = 0; i < uart_manage.existed_num; i++)
    {
        for (int j = 0; j < uart_manage.task_handle[i].task_num; j++)
        {
            vTaskDelete(uart_manage.task_handle[i].handle[j]);
            uart_manage.task_num--;
        }
        uart_manage.task_handle[i].task_num = 0;   
    } 
    return UART_OK;
}


void uart_send(void *param)
{
    uart_init_t* uart_config = (uart_init_t*)param;
    uart_port_t uart_num = uart_config->uart_num;
    QueueHandle_t uart_queue = *uart_config->tx_buff_queue;
    while (1)
    {
        events event;
        // ESP_LOGE(UART_TAG, "&event: %p", &event);
        while (xQueueReceive(uart_queue, &event, pdMS_TO_TICKS(100)) != pdTRUE)
            ;
        if (event.buff_len != 0)
        {
            uart_write_bytes(uart_num, (const char *)event.buff, event.buff_len);
        }
    }
    vTaskDelete(NULL);
}

void uart_rev(void *param)
{
    uart_init_t* uart_config = (uart_init_t*)param;
    uart_port_t uart_num = uart_config->uart_num;
    char buffer[UART_BUF_SIZE];
    int uart_buf_len = 0;
    events event;
    QueueHandle_t uart_queue = *uart_config->rx_buff_queue;
    while (1)
    {
        uart_get_buffered_data_len(uart_num, (size_t *)&uart_buf_len);
        if (uart_buf_len)
        {
            uart_buf_len = uart_buf_len > UART_BUF_SIZE ? UART_BUF_SIZE : uart_buf_len;
            uart_buf_len = uart_read_bytes(uart_num, event.buff_arr, uart_buf_len, pdMS_TO_TICKS(5));
            event.buff_arr[uart_buf_len] = '\0';
            //ESP_LOGE(UART_TAG, "buffer = %s  \nuart_buf_len = %d\n", buffer, uart_buf_len);
            if (uart_buf_len != 0)
            {
                // strncpy(event.buff, buffer, uart_buf_len);
               // ESP_LOGE(UART_TAG, "event buffer = %s  \n", event.buff_arr);
                event.buff = event.buff_arr;
                event.buff_len = uart_buf_len;
                uart_buf_len = 0;
                if (xQueueSend(uart_queue, &event, pdMS_TO_TICKS(10)) == pdPASS)
                {
                    //ESP_LOGE(UART_TAG, "SEND TO QUEUE\n");
                }
                else
                {
                    ESP_LOGE(UART_TAG, "SEND TO QUEUE FAILD\n");
                }
            }
        }
        else
        {
            vTaskDelay(5);
        }
        
    }
    vTaskDelete(NULL);
}

