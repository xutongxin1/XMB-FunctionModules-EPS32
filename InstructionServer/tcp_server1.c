#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "InstructionServer/wifi_configuration.h"
#include "InstructionServer/nvs_api.h"

#include "other/elaphureLink_protocol.h"

#include "SwitchMode/Handle.h"

#include "TCP-CH/tcp.h"

#include "UART/uart_val.h"
#include "UART/uart_config.h"
#include "UART/uart_analysis_parameters.h"

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




static int Flag1 = 0;               //是否收到COM心跳包
const char kHeartRet[5] = "OK!\r\n"; //心跳包发送
static int Flag3 = 0;               //是否发送OK心跳包
int sendFlag = 0;
char modeRet[5] = "RF0\r\n"; //心跳包发送
extern int Command_Flag; 
static const char *TAG = "example";

TaskHandle_t kDAPTaskHandle1 = NULL;
int kRestartDAPHandle1 = NO_SIGNAL;

uint8_t kState1 = ACCEPTING;
int kSock1 = -1;
int written = 0;
int keepAlive = 1;
int keepIdle = KEEPALIVE_IDLE;
int keepInterval = KEEPALIVE_INTERVAL;
int keepCount = KEEPALIVE_COUNT;

extern uart_configrantion c1;
extern uart_configrantion c2;
extern uart_configrantion c3;
extern bool c1UartConfigFlag;
extern bool c2UartConfigFlag;
extern bool uart_handle_flag;

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

                if (len == 0) {
                    printf("Connection closed\r\n");
                    break;
                }
                    // Data received
                else {

                    switch (kState1) {
                        case ACCEPTING:kState1 = ATTACHING;

                        case ATTACHING:
                            printf("RX: %s\n", tcp_rx_buffer);
                            heart_beat(len, tcp_rx_buffer); //当COM心跳成功发送，则置Falg为1
                            printf("Flag1=%d,Flag3=%d\n", Flag1, Flag3);

                            if (Flag3 == 1 && Flag1 == 1 &&tcp_rx_buffer[0]=='{') {
                                printf("\nCommand analysis!!!\n");

                                Flag1 = 0;
                                command_json_analysis(len, tcp_rx_buffer, kSock1);

                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                            }

                            if (Flag1 == 1) //确认接收到COM心跳
                            {

                                written = send(kSock1, kHeartRet, 5, 0);

                                Flag3 = 1; //确认好心跳OK发送完整

                            }
                            
                            break;
                        default:
                        printf("unkonw kstate!\r\n");
                    }
                }
            }
            // kState = ACCEPTING;
            if (kSock1 != -1) {
                printf("Shutting down socket and restarting...\r\n");
                close(kSock1);
                if (kState1 == EMULATING || kState1 == EL_DATA_PHASE) {
                    kState1 = ACCEPTING;
                }


                kRestartDAPHandle1 = RESET_HANDLE;
                if (kDAPTaskHandle1)
                    xTaskNotifyGive(kDAPTaskHandle1);


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
    c2UartConfigFlag=false;
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
                
                if(str_command==220 && uart_handle_flag==true)
                {   
                    if(uart_1_parameter_analysis(pattach,&c1)){
                        c1UartConfigFlag=true;
                    }
                    
                    if(uart_2_parameter_analysis(pattach,&c2)){
                        c2UartConfigFlag=true;
                    }

                    if(c1UartConfigFlag==true&&c2UartConfigFlag==true){
                        uart_task();
                    }

                    cJSON_Delete(pJsonRoot);
                }
                else if (str_command == 101) {

                    strattach = pattach->valuestring;
                    
                    str_attach = (*strattach);
                    
                    if (str_attach != '0' && str_attach <= '9' && str_attach >= '1') {
                        printf("\n%c\n", str_attach);
                        nvs_flash_write(str_attach, ksock);
                        do{
                            written=send(ksock, kHeartRet, 5, 0);
                            printf("%d\n",written);
                        }while(written<=0);
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





