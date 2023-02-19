#include "nvs_flash.h"
#include "InstructionServer/nvs_api.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


extern int sendFlag;
extern char modeRet[5];
extern int kSock1 ;
int Command_Flag = 0;               //指令模式

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