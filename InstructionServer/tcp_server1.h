#ifndef __TCP_SERVER1_H__
#define __TCP_SERVER1_H__

#define PORT                        1920
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

typedef struct {
  int mode;
  char *band;
  char *parity;
  int data;
  int stop;

} Uart_parameter_Analysis;

enum state_t {
  ACCEPTING,
  ATTACHING,
  EMULATING,
  EL_DATA_PHASE
};

enum reset_handle_t
{
    NO_SIGNAL = 0,
    RESET_HANDLE = 1,
    DELETE_HANDLE = 2,
};///////////

void tcp_server_task_1(void *pvParameters);
void heart_beat(unsigned int len, char *rx_buffer);
void command_json_analysis(unsigned int len, void *rx_buffer, int ksock);
void attach_status(char str_attach);
void nvs_flash_write(char mode_number, int listen_sock);
void nvs_flash_read(int listen_sock);
int uart_c_1_parameter_analysis(void *attach_rx_buffer, Uart_parameter_Analysis *t);
int uart_c_2_parameter_mode(void *attach_rx_buffer, Uart_parameter_Analysis *t);
int uart_c_3_parameter_analysis(void *attach_rx_buffer, Uart_parameter_Analysis *t);
enum Command_mode {
  DAP = 1,
  UART,
  ADC,
  DAC,
  PWM_Collect,
  PWM_Simulation,
  I2C,
  SPI,
  CAN
};

#endif