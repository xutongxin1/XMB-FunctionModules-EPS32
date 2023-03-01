#ifndef __NVS_API_H__
#define __NVS_API_H__

void nvs_flash_write(char mode_number, int listen_sock);
int nvs_flash_read(int listen_sock);

#endif