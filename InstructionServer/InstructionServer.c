#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "NVS/nvs_api.h"

#include "SwitchMode/SwitchModeHandle.h"

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
#include "InstructionServer.h"
#include "WirelessDAP_main/DAP_handle.h"
#include "WirelessDAP_main/usbip_server.h"

//static int receive_com_flag = 0;                // 是否收到COM心跳包
const char kHeartRet[5] = "OK!\r\n"; // 心跳包发送
//static int send_ok_flag = 0;                // 是否发送OK心跳包

char modeRet[5] = "RF0\r\n"; // 心跳包发送
char First_Ret[5] = "SF0\r\n";

//int first_receive_flag = 1;//是否是第一次接收到心跳包
static const char *TAG = "TCPInstructionTask";

TaskHandle_t kDAPTaskHandle1 = NULL;

uint8_t kState1 = 0;
int instrucion_kSock = -1;

int keepAlive = 1;
int keepIdle = KEEPALIVE_IDLE;
int keepInterval = KEEPALIVE_INTERVAL;
int keepCount = KEEPALIVE_COUNT;

extern UartInitT c1;
extern UartInitT c2;
extern UartInitT c3;
extern bool c1UartConfigFlag;
extern bool c2UartConfigFlag;


void TCPInstructionTask(void) {

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
        if (listen_sock < 0)
        {
            printf("Unable to create socket: errno %d\r\n", errno);
            break;
        }
        printf("Socket created\r\n");

        setsockopt(listen_sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
        setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));//绑定socket
        if (err != 0)
        {
            printf("Socket unable to bind: errno %d\r\n", errno);
            break;
        }
        printf("Socket binded\r\n");

        err = listen(listen_sock, 1);//监听端口信息
        if (err != 0)
        {
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
        while (1)//等待客户端连接
        {

            instrucion_kSock = accept(listen_sock, (struct sockaddr *) &source_addr, &addr_len);//接收来自客户端的连接要求，返回新套接字的句柄
            if (instrucion_kSock < 0) {
                printf(" Unable to accept connection: errno %d\r\n", errno);
                break;
            }
            // printf("1");
            setsockopt(instrucion_kSock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(instrucion_kSock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(instrucion_kSock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(instrucion_kSock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
            setsockopt(instrucion_kSock, IPPROTO_TCP, TCP_NODELAY, (void *) &on, sizeof(on));
            printf("Socket accepted %d\r\n", instrucion_kSock);

            if (need_send_RF)//重启后读取flash的值，读到的值放进Command_Flag
            {
                need_send_RF = 0;

                int send_bytes;
                do {
                    send_bytes = send(instrucion_kSock, modeRet, 5, 0);

                    ESP_LOGI(TAG, "send RF%C finish%d", modeRet[2], send_bytes);

                } while (send_bytes < 0);
            }

            while (1)//循环接收数据
            {

                char tcp_rx_buffer[1500] = {0};

                int len = recv(instrucion_kSock, tcp_rx_buffer, sizeof(tcp_rx_buffer), 0);//从socket中读取字符到tcp_rx_buffer

                if (len == 0) {
                    printf("Connection closed\r\n");
                    break;
                }
                    // Data received
                else {

                    switch (kState1) {
                        case ACCEPTING:kState1 = ATTACHING;

                        case ATTACHING:
                            // printf("RX: %s\n", tcp_rx_buffer);
                            HeartBeat(len, tcp_rx_buffer); // 发送心跳包，当COM心跳成功发送，则置Flag为1
                            //printf("receive_com_flag=%d,send_ok_flag=%d\nfirst_receive_flag=%d,Command_Flag=%d\n", receive_com_flag, send_ok_flag,first_receive_flag,Command_Flag);

                            if (tcp_rx_buffer[0] == '{')//已经接收到com心跳包，发送ok，并且接收到的指令正常
                            {
                                ESP_LOGI(TAG, "Analyze Command");

//                                receive_com_flag = 0;
                                CommandJsonAnalysis(len, tcp_rx_buffer, instrucion_kSock);

                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                            }
//                            // first_receive_flag=Command_Flag;
//                            if (receive_com_flag == 1 && Command_Flag == 0 && first_receive_flag == 0) {
//                                send_ok_flag = 1;//
//                            }
//
//                            if (receive_com_flag == 1 && Command_Flag != 0 && first_receive_flag == 0) // 接收到COM心跳，接收到指令
//                            {
////                            written = send(kSock1, kHeartRet, 5, 0);//发送ok        这个written用来干嘛，都没使用过
//                                int written = send(kSock1, kHeartRet, 5, 0);//发送ok
//                                send_ok_flag = 1; // 已经发送ok
//                            }
//
//                            if (receive_com_flag == 1 && Command_Flag == 0 && first_receive_flag == 1) // 第一次确认接收到COM心跳
//                            {
//                                int written = send(kSock1, kHeartRet, 5, 0);
//                                first_receive_flag = 0;
//                                send_ok_flag = 1;
//                            }

                            break;
                        default:printf("unkonw kstate!\r\n");
                    }
                }
            }
            // kState = ACCEPTING;
            if (instrucion_kSock != -1)//关掉重启
            {
                printf("Shutting down socket and restarting...\r\n");
                close(instrucion_kSock);
                if (kState1 == EMULATING || kState1 == EL_DATA_PHASE)
                {
                    kState1 = ACCEPTING;//修改为默认接收模式
                }

//                kRestartDAPHandle = RESET_HANDLE;//?
//                if (kDAPTaskHandle1)
//                    xTaskNotifyGive(kDAPTaskHandle1);
            }
        }
    }
    vTaskDelete(NULL);
}

void HeartBeat(unsigned int len, char *rx_buffer)
{
    const char *p1 = "COM\r\n";
    if (rx_buffer != NULL)
    {
        if (strstr(rx_buffer, p1))//接收到p1
        {
            send(instrucion_kSock, kHeartRet, 5, 0);
        }
    }
}

void CommandJsonAnalysis(unsigned int len, void *rx_buffer, int ksock)
{
    int send_bytes = 0;//发送字符的字节数
    char *strattach = NULL;
    char str_attach;
    int str_command;
    c1UartConfigFlag = false;
    c2UartConfigFlag = false;
    // 首先整体判断是否为一个json格式的数据

    cJSON *p_json_root = cJSON_Parse(rx_buffer);

    cJSON *pcommand = cJSON_GetObjectItem(p_json_root, "command"); // 解析command字段内容

    cJSON *pattach = cJSON_GetObjectItem(p_json_root, "attach"); // 解析attach字段内容

    // 是否为json格式数据
    if (p_json_root != NULL && pcommand != NULL && pattach != NULL) {

        str_command = pcommand->valueint;
        ESP_LOGI(TAG, "str_command : %d", str_command);

        //优先解析全局指令包
        if (str_command == 101) {

            strattach = pattach->valuestring;

            str_attach = (*strattach);

            if (str_attach <= '9' && str_attach >= '1')//刚开机，无需修改NVS
            {
                printf("\n%c\n", str_attach);
                if (working_mode == NONE_MODE) {

                    //NVSFlashWrite(str_attach, ksock);
                    First_Ret[2] = str_attach;
                    do {
                        send_bytes = send(instrucion_kSock, First_Ret, 5, 0);

                        ESP_LOGI(TAG, "send SF%C finish%d", First_Ret[2], send_bytes);

                    } while (send_bytes < 0);
                    ChangeWorkMode(str_attach - '0');//配置对应的模式（此处字符串需要减0）
//                    send_ok_flag = 0;
                } else //修改模式
                {
                    NVSFlashWrite(str_attach - '0');
                    do {
                        send_bytes = send(ksock, kHeartRet, 5, 0);
                        ESP_LOGI(TAG, "Change Success");
                    } while (send_bytes <= 0);
                    int s_1 = shutdown(ksock, 0);
                    int s_2 = close(ksock);
                    ESP_LOGI(TAG, "You are closing the connection %d %d %d.", instrucion_kSock, s_1, s_2);
                    ESP_LOGI(TAG, "Restarting now.");
                    cJSON_Delete(p_json_root);
                    fflush(stdout);
                    esp_restart(); // 断电重连
                }
            }
        }

        //再解析各个模式的指令包
        switch (working_mode) {

            case NONE_MODE:break;
            case DAP:break;
            case UART: {
                if (str_command == 220)//接收到的信息要求配置为串口模式以后收到220
                {
                    if (Uart1ParameterAnalysis(pattach, &c1)) {
                        c1UartConfigFlag = true;
                    }

                    if (Uart2ParameterAnalysis(pattach, &c2)) {
                        c2UartConfigFlag = true;
                    }

                    // if(c1UartConfigFlag==true&& c2UartConfigFlag==true){
                    UartTask(ksock);
                    //}

                    cJSON_Delete(p_json_root);
                }
                break;
            }
                break;

            case ADC:break;
            case DAC:break;
            case PWM_COLLECT:break;
            case PWM_SIMULATION:break;
            case I2C:break;
            case SPI:break;
            case CAN:break;
        }

    }
}

void ChangeWorkMode(char mode) {

    switch (mode) {
        case DAP:DAPHandle();
            break;
        case UART:UartHandle();
            break;
        case ADC:ADCHandle();
            break;
        case DAC:DACHandle();
            break;
        case PWM_COLLECT:PwmCollectHandle();
            break;
        case PWM_SIMULATION:PwmSimulationHandle();
            break;
        case I2C:I2CHandle();
            break;
        case SPI:SpiHandle();
            break;
        case CAN:CanHandle();
            break;
        default:break;
    }
}
