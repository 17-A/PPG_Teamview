#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "sdcard.h"

void app_main()
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };

    sdmmc_card_t* sdcard = NULL;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 10000;
    spi_bus_config_t bus_config = {
        .mosi_io_num = CONFIG_PIN_NUM_MOSI,
        .miso_io_num = CONFIG_PIN_NUM_MISO,
        .sclk_io_num = CONFIG_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();

    esp_err_t result = sdcard_initialize(&mount_config, &sdcard, &host, &bus_config, &slot_config);


    if (result == ESP_OK)
    {
        result = sdcard_createFolder("Test_getfilepath");
        result = sdcard_createDeviceStructure("Test_getfilepath");
        result = sdcard_getFilePath("Log.log");
    }
    else
    {
       printf("ERROR");
    }
}
