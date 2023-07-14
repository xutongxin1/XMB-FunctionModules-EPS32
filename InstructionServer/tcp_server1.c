#include <string.h>
#include <stdint.h>
#include <sys/param.h>

#include "InstructionServer/wifi_configuration.h"
#include "NVS/nvs_api.h"

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

static int receive_com_flag = 0;                // 是否收到COM心跳包
const char kHeartRet[5] = "OK!\r\n"; // 心跳包发送
static int send_ok_flag = 0;                // 是否发送OK心跳包

char modeRet[5] = "RF0\r\n"; // 心跳包发送
char First_Ret[5] = "SF0\r\n";
extern int Command_Flag;//指令command的内容
int first_receive_flag = 1;//是否是第一次接收到心跳包
static const char *TAG = "example";

TaskHandle_t kDAPTaskHandle1 = NULL;
int kRestartDAPHandle1 = NO_SIGNAL;

uint8_t kState1 = ACCEPTING;
int kSock1 = -1;
//int written = 0;
int keepAlive = 1;
int keepIdle = KEEPALIVE_IDLE;
int keepInterval = KEEPALIVE_INTERVAL;
int keepCount = KEEPALIVE_COUNT;

extern UartInitT c1;
extern UartInitT c2;
extern UartInitT c3;
extern bool c1UartConfigFlag;
extern bool c2UartConfigFlag;
extern bool uart_handle_flag;

void TcpCommandPipeTask(void)
{

    char addr_str[128];
    int addr_family;
    int ip_protocol;

    int on = 1;
    while (1)
    {

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
        while (1)
        {

            kSock1 = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);//接收来自客户端的连接要求，返回新套接字的句柄
            if (kSock1 < 0)
            {
                printf(" Unable to accept connection: errno %d\r\n", errno);
                break;
            }
            // printf("1");
            setsockopt(kSock1, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
            setsockopt(kSock1, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on));
            printf("Socket accepted %d\r\n", kSock1);

            if (NvsFlashRead())//重启后读取flash的值，读到的值放进Command_Flag
            {
                AttachStatus(Command_Flag+'0');//配置为重启前写入flash的模式
            }

            while (1)
            {

                char tcp_rx_buffer[1500] = {0};

                int len = recv(kSock1, tcp_rx_buffer, sizeof(tcp_rx_buffer), 0);//从socket中读取字符到tcp_rx_buffer

                if (len == 0)
                {
                    printf("Connection closed\r\n");
                    break;
                }
                // Data received
                else
                {

                    switch (kState1)
                    {
                    case ACCEPTING:
                        kState1 = ATTACHING;

                    case ATTACHING:
                        // printf("RX: %s\n", tcp_rx_buffer);
                        HeartBeat(len, tcp_rx_buffer); // 发送心跳包，当COM心跳成功发送，则置Flag为1
                        //printf("receive_com_flag=%d,send_ok_flag=%d\nfirst_receive_flag=%d,Command_Flag=%d\n", receive_com_flag, send_ok_flag,first_receive_flag,Command_Flag);

                        if (send_ok_flag == 1 && receive_com_flag == 1 && tcp_rx_buffer[0] == '{')//已经接收到com心跳包，发送ok，并且接收到的指令正常
                        {
                            printf("\nCommand analysis!!!\n");

                            receive_com_flag = 0;
                            CommandJsonAnalysis(len, tcp_rx_buffer, kSock1);

                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                        }
                        // first_receive_flag=Command_Flag;
                        if (receive_com_flag == 1 && Command_Flag == 0 && first_receive_flag == 0)
                        {
                            send_ok_flag = 1;//
                        }

                        if (receive_com_flag == 1 && Command_Flag != 0 && first_receive_flag == 0) // 接收到COM心跳，接收到指令
                        {
//                            written = send(kSock1, kHeartRet, 5, 0);//发送ok        这个written用来干嘛，都没使用过
                            int written = send(kSock1, kHeartRet, 5, 0);//发送ok
                            send_ok_flag = 1; // 已经发送ok
                        }

                        if (receive_com_flag == 1 && Command_Flag == 0 && first_receive_flag == 1) // 第一次确认接收到COM心跳
                        {
                            int written = send(kSock1, kHeartRet, 5, 0);
                            first_receive_flag = 0;
                            send_ok_flag = 1;
                        }

                        break;
                    default:
                        printf("unkonw kstate!\r\n");
                    }
                }
            }
            // kState = ACCEPTING;
            if (kSock1 != -1)//关掉重启
            {
                printf("Shutting down socket and restarting...\r\n");
                close(kSock1);
                if (kState1 == EMULATING || kState1 == EL_DATA_PHASE)
                {
                    kState1 = ACCEPTING;//修改为默认接收模式
                }

                kRestartDAPHandle1 = RESET_HANDLE;
                if (kDAPTaskHandle1)
                    xTaskNotifyGive(kDAPTaskHandle1);
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
            receive_com_flag = 1;
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

    printf("\nIn analysis\n");
    // 是否为json格式数据
    if (p_json_root != NULL)
    {
        // printf("\nlife1\n");
        // 是否指令为空
        if (pcommand != NULL)
        {
            // printf("\nlife2\n");
            // 是否附加信息为空
            if (pattach != NULL)
            {

                str_command = pcommand->valueint;
                printf("\nstr_command : %d\n", str_command);

                if (str_command == 220 && uart_handle_flag == true)//接收到的信息要求配置为串口模式以后收到220
                {
                    if (Uart1ParameterAnalysis(pattach, &c1))
                    {
                        c1UartConfigFlag = true;
                    }

                    if (Uart2ParameterAnalysis(pattach, &c2))
                    {
                        c2UartConfigFlag = true;
                    }

                    // if(c1UartConfigFlag==true&& c2UartConfigFlag==true){
                    UartTask(ksock);
                    //}

                    cJSON_Delete(p_json_root);
                }
                else if (str_command == 101)
                {

                    strattach = pattach->valuestring;

                    str_attach = (*strattach);

                    if (str_attach <= '9' && str_attach >= '1' && Command_Flag == 0)//刚开机
                    {
                        printf("\n%c\n", str_attach);
                        //NvsFlashWrite(str_attach, ksock);
                        First_Ret[2] = str_attach;
                        Command_Flag=str_attach-'0';
                        do
                        {
                            send_bytes = send(kSock1, First_Ret, 5, 0);

                            printf("\nsend SF%C finish%d\n", First_Ret[2], send_bytes);

                        } while (send_bytes < 0);
                        AttachStatus(str_attach);//配置对应的模式
                        send_ok_flag = 0;
                    }
                    else if (str_attach <= '9' && str_attach >= '1' && Command_Flag != 0)//修改模式
                    {
                        printf("\n%c\n", str_attach);
                        NvsFlashWrite(str_attach);
                        do
                        {
                            send_bytes = send(ksock, kHeartRet, 5, 0);
                            printf("%d\n", send_bytes);
                        } while (send_bytes <= 0);
                        int s_1 = shutdown(ksock, 0);
                        int s_2 = close(ksock);
                        printf("\nYou are closing the connection %d %d %d.\n", kSock1, s_1, s_2);
                        printf("Restarting now.\n");
                        cJSON_Delete(p_json_root);
                        fflush(stdout);
                        esp_restart(); // 断电重连
                    }
                }
            }
        }
    }
}


void AttachStatus(char str_attach)
{
    int attach = (int)str_attach;

    attach = attach - '0';
    switch (attach)
    {
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
    default:
        break;
    }
}
