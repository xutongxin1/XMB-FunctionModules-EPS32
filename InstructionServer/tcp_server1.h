#ifndef __TCP_SERVER1_H__
#define __TCP_SERVER1_H__

#define PORT                        1920
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

// typedef struct uart_configrantion
// {
//   // int CH;
//   // int mode;
//   // char *band;
//   // char *parity;
//   // int data;
//   // int stop;
//   QueueHandle_t* uart_queue;
//   struct uart_pin pin;
//   uart_port_t uart_num;
//   enum UartIOMode mode;
//   uart_config_t uart_config;

// } ;
#include "UART/uart_val.h"
#include "UART/uart_config.h"

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