#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "InstructionServer/wifi_configuration.h"

#include "other/elaphureLink_protocol.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <errno.h>

#include "cJSON.h"
#include "tcp_server1.h"
#include "SwitchMode/Handle.h"

#include "UART/bps_config.h"
#include "UART/uart_config.h"

static int Flag1 = 0;               //是否收到COM心跳包
const char kHeartRet[5] = "OK!\r\n"; //心跳包发送
static int Flag3 = 0;               //是否发送OK心跳包
int Command_Flag = 0;               //指令模式
int sendFlag = 0;
char modeRet[5] = "RF0\r\n"; //心跳包发送
struct uart_configrantion c1;
struct uart_configrantion c2;
struct uart_configrantion c3;
static const char *TAG = "example";

TaskHandle_t kDAPTaskHandle1 = NULL;
int kRestartDAPHandle1 = NO_SIGNAL;

uint8_t kState1 = ACCEPTING;
volatile int kSock1 = -1;
int written = 0;
int keepAlive = 1;
int keepIdle = KEEPALIVE_IDLE;
int keepInterval = KEEPALIVE_INTERVAL;
int keepCount = KEEPALIVE_COUNT;
bool c1UartConfigFlag = false; 
bool c3UartConfigFlag = false;

void tcp_server_task_1(void *pvParameters) {

    char addr_str[128];
    int addr_family;
    int ip_protocol;

    int on = 1;
    while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 dest_addr;
        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            printf("Unable to create socket: errno %d\r\n", errno);
            break;
        }
        printf("Socket created\r\n");

        setsockopt(listen_sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &on, sizeof(on));
        setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (void *) &on, sizeof(on));

        int err = bind(listen_sock, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
        if (err != 0) {
            printf("Socket unable to bind: errno %d\r\n", errno);
            break;
        }
        printf("Socket binded\r\n");

        err = listen(listen_sock, 1);
        if (err != 0) {
            printf("Error occured during listen: errno %d\r\n", errno);
            break;
        }
        printf("Socket listening\r\n");

#ifdef CONFIG_EXAMPLE_IPV6
        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
#else
        struct sockaddr_in source_addr;
#endif
        uint32_t addr_len = sizeof(source_addr);
        while (1) {

            kSock1 = accept(listen_sock, (struct sockaddr *) &source_addr, &addr_len);
            if (kSock1 < 0) {
                printf(" Unable to accept connection: errno %d\r\n", errno);
                break;
            }
            // printf("1");
            setsockopt(kSock1, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_NODELAY, (void *) &on, sizeof(on));
            printf("Socket accepted %d\r\n", kSock1);

            if(nvs_flash_read(listen_sock)){
            attach_status(Command_Flag);
            }

            while (1) {
                char tcp_rx_buffer[1500] = {0};

                int len = recv(kSock1, tcp_rx_buffer, sizeof(tcp_rx_buffer), 0);

                // // Error occured during receiving
                // if (len < 0)
                // {
                //     printf("recv failed: errno %d\r\n", errno);
                //     break;
                // }
                // // Connection closed
                // else 
                if (len == 0) {
                    printf("Connection closed\r\n");
                    break;
                }
                    // Data received
                else {
                    // #ifdef CONFIG_EXAMPLE_IPV6
                    //                     // Get the sender's ip address as string
                    //                     if (source_addr.sin6_family == PF_INET)
                    //                     {
                    //                         inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                    //                     }
                    //                     else if (source_addr.sin6_family == PF_INET6)
                    //                     {
                    //                         inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                    //                     }
                    // #else
                    //                     inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                    // #endif

                    switch (kState1) {
                        case ACCEPTING:kState1 = ATTACHING;

                        case ATTACHING:
                            printf("RX: %s\n", tcp_rx_buffer);
                            heart_beat(len, tcp_rx_buffer); //当COM心跳成功发送，则置Falg为1
                            printf("Flag1=%d,Flag3=%d\n", Flag1, Flag3);

                            if (Flag3 == 1 && Flag1 == 1 &&tcp_rx_buffer[0]=='{') {
                                printf("\nCommand analysis!!!\n");
                                //printf("the data is %s\n", tcp_rx_buffer);
                                //printf("Flag1=%d,Flag3=%d\n", Flag1, Flag3);
                                Flag1 = 0;
                                command_json_analysis(len, tcp_rx_buffer, kSock1);

                                //close(listen_sock);

                                // //vTaskDelete(NULL);
                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                            }

                            if (Flag1 == 1) //确认接收到COM心跳
                            {

                                written = send(kSock1, kHeartRet, 5, 0);

                                Flag3 = 1; //确认好心跳OK发送完整

                                //printf("Flag1=%d,Flag3=%d", Flag1, Flag3);
                            }
                            

                            // attach(tcp_rx_buffer, len);
                            break;
                        default:
                        printf("unkonw kstate!\r\n");
                    }
                }
            }
            // kState = ACCEPTING;
            if (kSock1 != -1) {
                printf("Shutting down socket and restarting...\r\n");
                // shutdown(kSock, 0);
                close(kSock1);
                if (kState1 == EMULATING || kState1 == EL_DATA_PHASE) {
                    kState1 = ACCEPTING;
                }

                // Restart DAP Handle
                // el_process_buffer_free();

                kRestartDAPHandle1 = RESET_HANDLE;
                if (kDAPTaskHandle1)
                    xTaskNotifyGive(kDAPTaskHandle1);

                // shutdown(listen_sock, 0);
                // close(listen_sock);
                // vTaskDelay(5);
            }
        }
    }
    vTaskDelete(NULL);
}

void heart_beat(unsigned int len, char *rx_buffer) {
    const char *p1 = "COM\r\n";
    if (rx_buffer != NULL) {
        if (strstr(rx_buffer, p1)) {
            Flag1 = 1;
        }
    }
}

void command_json_analysis(unsigned int len, void *rx_buffer, int ksock) {
    char *strattach = NULL;
    char str_attach;
    int  str_command;
    c1UartConfigFlag=false;
    c3UartConfigFlag=false;
    //首先整体判断是否为一个json格式的数据

    cJSON *pJsonRoot = cJSON_Parse(rx_buffer);

    cJSON *pcommand = cJSON_GetObjectItem(pJsonRoot, "command"); // 解析command字段内容

    cJSON *pattach = cJSON_GetObjectItem(pJsonRoot, "attach");   // 解析attach字段内容

    printf("\nIn analysis\n");
    //是否为json格式数据
    if (pJsonRoot != NULL) {
        // printf("\nlife1\n");
        //是否指令为空
        if (pcommand != NULL) {
            // printf("\nlife2\n");
            //是否附加信息为空
            if (pattach != NULL) {

                str_command = pcommand->valueint;
                printf("\nstr_command : %d", str_command);
                //printf("\nthe data is %c", *strattach);
                // printf("\nfree\n");
                
                if(str_command==220)//二级配置指令
                {   
                    if(uart_c_1_parameter_analysis(pattach,&c1)){//通道一串口 2 接收到命令
                        c1UartConfigFlag=true;//确定配置通道一
                    }
                    
                    if(uart_c_3_parameter_analysis(pattach,&c3)){//通道三 串口 0 接收到命令
                        c3UartConfigFlag=true;//确定配置通道三
                    }

                    if(c1UartConfigFlag==true&&c3UartConfigFlag==true){//一三两个通道都需要配置
                        if(uart_c_2_parameter_mode(pattach,&c2,&c1,&c3)){//启用通道二
                            printf("c2 setup:\n");
                            uart_setup(&c2);
                        }
                    }

                    if(c1UartConfigFlag==true){
                        printf("c1 setup:\n");
                        uart_setup(&c1);   
                        printf("DONE\n");
                    }

                    if(c3UartConfigFlag==true){
                        printf("c3 setup:\n");
                        uart_setup(&c3);
                        printf("DONE\n");
                    }    
                    cJSON_Delete(pJsonRoot);
                }
                if (str_command == 101) {
                    strattach = pattach->valuestring;
                    str_attach = (*strattach);
                    if (str_attach != '0' && str_attach <= '9' && str_attach >= '1') {
                        printf("\n%c\n", str_attach);
                        nvs_flash_write(str_attach, ksock);
                        send(ksock, kHeartRet, 5, 0);
                        int s_1 = shutdown(ksock, 0);
                        int s_2 = close(ksock);
                        printf("\nYou are closing the connection %d %d %d.\n", kSock1, s_1, s_2);
                        printf("Restarting now.\n");
                        cJSON_Delete(pJsonRoot);
                        fflush(stdout);
                        esp_restart(); //重启函数，esp断电重连
                    }
                }
                Flag3 = 0;

            }
        }
    }
}

int uart_c_1_parameter_analysis(void *attach_rx_buffer,struct uart_configrantion* uartconfig) {//通道1  串口2 字符解析
    char *str_c_1 = NULL;
    unsigned char str_C1 = '0';
    cJSON *pc1 = cJSON_GetObjectItem(attach_rx_buffer, "c_1"); // 解析c1字段内容
    printf("\nc1:\n");
         //是否指令为空
         if (pc1 != NULL)
         {
            cJSON * item;

            uartconfig->pin.CH=CH1;//通道一
            uartconfig->pin.MODE = RX;//接收模式//通道1 只收不发

            uartconfig->uart_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;

            uartconfig->uart_num = UART_NUM_2;

            item=cJSON_GetObjectItem(pc1,"mode");
            uartconfig->mode = item->valueint;
            printf("mode = %d\n",uartconfig->mode);

            if(uartconfig->mode!=Input&&uartconfig->mode!=SingleInput){//通道1 只能使用输入或者独占输入
                return 0;
            }
            item=cJSON_GetObjectItem(pc1,"band");
            uartconfig->uart_config.baud_rate = item->valueint;
            printf("band = %d\n",uartconfig->uart_config.baud_rate);
            item=cJSON_GetObjectItem(pc1,"parity");
            uartconfig->uart_config.parity = item->valueint;
            printf("parity = %d\n",uartconfig->uart_config.parity);

            item=cJSON_GetObjectItem(pc1,"data");
            uartconfig->uart_config.data_bits = (item->valueint)-5; // 为什么减 5
            printf("data = %d\n",uartconfig->uart_config.data_bits);

            item=cJSON_GetObjectItem(pc1,"stop");
            uartconfig->uart_config.stop_bits=item->valueint;
            printf("stop = %d\n",uartconfig->uart_config.stop_bits);

            return 1;
         }
     
    return 0;
}

int uart_c_2_parameter_mode(void *attach_rx_buffer,struct uart_configrantion* c2,struct uart_configrantion* c1,struct uart_configrantion* c3)
{//通道2可以配置为只收或只发或收发   //  串口 2
    int strC2=0;
    //首先整体判断是否为一个json格式的数据
    cJSON *pc2 = cJSON_GetObjectItem(attach_rx_buffer, "c_2"); // 解析c2字段内容
    printf("\nc2:\n");
    //是否为json格式数据
        // printf("\nlife1\n");
        //是否指令为空
        if (pc2 != NULL)
        {
            cJSON * item;
            item=cJSON_GetObjectItem(pc2,"mode");
            c2->mode = item->valueint;
            printf("mode = %d\n",c2->mode);
            
            if(c2->mode!=Follow1Output&&c2->mode!=Follow3Input){//跟随通道 1 输入或者跟随通道 3 输出
                return 0;
            }
            
            c2->pin.CH = CH2;
            c2->uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
            c2->uart_num = UART_NUM_2;

            switch (c2->mode)
            {
            case 5://发送模式//follow1output
            c2->uart_config.baud_rate = c1->uart_config.baud_rate;
            printf("baud=%d\n",c2->uart_config.baud_rate);
            c2->uart_config.parity = c1->uart_config.parity;
            printf("parity=%d\n",c2->uart_config.parity);
            c2->uart_config.data_bits = c1->uart_config.data_bits;
            printf("data=%d\n",c2->uart_config.data_bits);
            c2->uart_config.stop_bits = c1->uart_config.stop_bits;
            printf("stop=%d\n",c2->uart_config.stop_bits);
            c2->pin.MODE = TX;
            break;

            case 6://接收模式//follow3input
            c2->uart_config.baud_rate = c3->uart_config.baud_rate ;
            printf("baud=%d\n",c2->uart_config.baud_rate);
            c2->uart_config.parity = c3->uart_config.parity;
            printf("parity=%d\n",c2->uart_config.parity);
            c2->uart_config.data_bits = c3->uart_config.data_bits;
            printf("data=%d\n",c2->uart_config.data_bits);
            c2->uart_config.stop_bits = c3->uart_config.stop_bits;
            printf("stop=%d\n",c2->uart_config.stop_bits);
            c2->pin.MODE = RX;
            break;
                TODO:  //case ://转发模式

            default:
                break;
            }
            return 1;
        }
    
    return 0;
}

int uart_c_3_parameter_analysis(void *attach_rx_buffer,struct uart_configrantion* uartconfig) {
    char *str_c_3 = NULL;
    //首先整体判断是否为一个json格式的数据
    cJSON *pc3 = cJSON_GetObjectItem(attach_rx_buffer, "c_3"); // 解析通道3 字段内容
    printf("\nc3:\n");
        //是否指令为空
        if (pc3 != NULL)
        {
            cJSON * item;

            uartconfig->pin.MODE = TX;

            item=cJSON_GetObjectItem(pc3,"mode");
            uartconfig->mode = item->valueint;
            printf("mode = %d\n",uartconfig->mode);            

            uartconfig->uart_num = UART_NUM_0;//通道三对应的是串口 0

            if(uartconfig->mode!=Output&&uartconfig->mode!=SingleOutput){//通道3只能配置为输出或者独占输出
                return 0;
            }

            item=cJSON_GetObjectItem(pc3,"band");
            uartconfig->uart_config.baud_rate = item->valueint;
            printf("band = %d\n",uartconfig->uart_config.baud_rate);
            
            item=cJSON_GetObjectItem(pc3,"parity");
            uartconfig->uart_config.parity = item->valueint;
            printf("parity = %d\n",uartconfig->uart_config.parity);
            
            item=cJSON_GetObjectItem(pc3,"data");
            uartconfig->uart_config.data_bits = (item->valueint)-5;
            printf("data = %d\n",uartconfig->uart_config.data_bits);
            
            item=cJSON_GetObjectItem(pc3,"stop");
            uartconfig->uart_config.stop_bits = item->valueint;
            printf("stop = %d\n",uartconfig->uart_config.stop_bits);
            
            uartconfig->pin.CH=CH3;

            uartconfig->uart_config.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;

            uartconfig->pin.MODE = TX;//输出模式

            return 1;
        }
    return 0;
}

void attach_status(char str_attach) {
    int attach = (int) str_attach;
    attach = attach - '0';
    switch (attach) {
        case DAP:DAP_Handle();
            break;
        case UART:UART_Handle();
            break;
        case ADC:ADC_Handle();
            break;
        case DAC:DAC_Handle();
            break;
        case PWM_Collect:PWM_Collect_Handle();
            break;
        case PWM_Simulation:PWM_Simulation_Handle();
            break;
        case I2C:I2C_Handle();
            break;
        case SPI:SPI_Handle();
            break;
        case CAN:CAN_Handle();
            break;
        default:break;
    }
}

void nvs_flash_write(char mode_number, int listen_sock) {
    // Initialize NVS
    nvs_flash_erase();
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Open
    printf("\n");
    // printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("openDone\n");

        // Write
        // printf("Updating restart counter in NVS ... ");
        err = nvs_set_i32(my_handle, "mode_number", mode_number);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        // printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }

    // printf("\n");

    // Restart module
    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}

int nvs_flash_read(int listen_sock){


    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open
    printf("\nopen\n");
    // printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {

        //printf("\nDone\n");

        // Read
        // printf("Reading restart counter from NVS ... ");
        char mode_number = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_i32(my_handle, "mode_number", &mode_number);

        switch (err) {
            case ESP_OK:
                // printf("Done\n");
                printf("mode_number = %c\n", mode_number);

                break;
            case ESP_ERR_NVS_NOT_FOUND:printf("The value is not initialized yet!\n");
                break;
            default:printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_flash_erase();
        // Close
        nvs_close(my_handle);
        //printf("\nnvs_close\n");

        Command_Flag = mode_number - '0';
        printf("Command %d\n", Command_Flag);

        if (Command_Flag != 0) {
            modeRet[2] = Command_Flag + '0';
            do {
                sendFlag = send(kSock1, modeRet, 5, 0);

                printf("\nsend RF%C finish%d\n", modeRet[2], sendFlag);

            } while (sendFlag < 0);
            Command_Flag = Command_Flag + '0';
            //printf("\nRF send finish\n");
            return 1;
        }
    }
    nvs_close(my_handle);
    return 0;
}
