#ifndef __Handle_H__
#define __Handle_H__
#include "TCP-CH/tcp.h"



void DAPHandle(void);
void UartHandle(void);
void ADCHandle(void);
void DACHandle(void);
void PwmCollectHandle(void);
void PwmSimulationHandle(void);
void I2CHandle(void);
void SpiHandle(void);
void CanHandle(void);


void UartTask(int ksock);

#endif