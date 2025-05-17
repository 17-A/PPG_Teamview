#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include "max30102.h"
#include "ssd1306.h"
#include "sdcard.h"  
#include "driver/i2c_types.h"

#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_SDA_IO     21       // Chân SDA bạn dùng
#define I2C_MASTER_SCL_IO     22       // Chân SCL bạn dùng
#define I2C_MASTER_FREQ_HZ    100000   // 100 kHz


//Cau hinh chan cho MAX30102
#define I2C_SDA_GPIO  21
#define I2C_SCL_GPIO  22
#define I2C_PORT      I2C_NUM_0
#define powerLed      UINT8_C(0x1F) //Cuong do led, tieu thu 6.4mA
#define sampleAverage 4
#define ledMode       2
#define sampleRate    500 //Tan so lay mau cao thi kich thuoc BUFFER_SIZE cung phai thay doi de co thoi gian thuat toan xu ly cac mau
#define pulseWidth    411 //Xung cang rong, dai thu duoc cang nhieu (18 bit)
#define adcRange      16384 //14 bit ADC tieu thu 65.2pA moi LSB

// ## đây là code ban đầu chạy được, để backup
// /**** MAX30102 ****/
// i2c_dev_t dev;  
// struct max30102_record record;
// unsigned long red, ir;
// TaskHandle_t readMAXTask_handle = NULL;

// static const char *TAG = "MAX30102";

// esp_err_t max30102_configure(void){
//     memset(&dev, 0, sizeof(i2c_dev_t));
//     ESP_ERROR_CHECK(max30102_initDesc(&dev, 0, I2C_SDA_GPIO, I2C_SCL_GPIO));
//     if(max30102_readPartID(&dev) == ESP_OK) {
//       ESP_LOGI(TAG, "Found MAX30102!");
//     }
//     else {
//       ESP_LOGE(TAG, "Not found MAX30102");
//     }
//     ESP_ERROR_CHECK(max30102_init(powerLed, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange, &record, &dev));
//     max30102_clearFIFO(&dev);
//     return ESP_OK;
// }

// void readMAX30102_task(void *pvParameter){
//     ESP_LOGI(TAG, "Bat dau doc cam bien AD8232");
  
//     while(1){
//         vTaskDelay(1);
//         max30102_check(&record, &dev); //Check the sensor, read up to 3 samples
//         while (max30102_available(&record)){
//             red = max30102_getFIFORed(&record);
//             ir = max30102_getFIFOIR(&record);
//             printf("%ld,%ld\n", red, ir);
//             max30102_nextSample(&record);
//         }
//     }
// }

// // ssd1306

// void app_main(void){
//     ESP_ERROR_CHECK(i2cdev_init()); 
//     if(max30102_configure() != ESP_OK){
//         ESP_LOGE(pcTaskGetName(NULL), "Khoi tao cam bien khong thanh cong, loi I2C...");
//     }else ESP_LOGI(pcTaskGetName(NULL), "Khoi tao cam bien thanh cong !");
    
//     xTaskCreatePinnedToCore(readMAX30102_task, "max30102_main", 1024 * 5, &readMAXTask_handle, configMAX_PRIORITIES - 1, NULL, 0);
// } ##



// Trung bình trượt & hiển thị
#define MA_WINDOW_SIZE 8
uint32_t ir_buffer[MA_WINDOW_SIZE] = {0};
int ir_index = 0;
int display_counter = 0;
int x_pos = 0;

static const char *TAG = "MAX30102_APP";

SSD1306_t oled;
struct max30102_record record;

// Scale giá trị IR thành pixel OLED
int map_value(uint32_t x, uint32_t in_min, uint32_t in_max, int out_min, int out_max) {
    if (x < in_min) x = in_min;
    if (x > in_max) x = in_max;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Trung bình trượt tín hiệu
uint32_t moving_average(uint32_t *buffer, int size) {
    uint64_t sum = 0;
    for (int i = 0; i < size; i++) {
        sum += buffer[i];
    }
    return sum / size;
}

void i2c_init_bus(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}

void max30102_main(void *pvParameter) {
    ESP_LOGI(TAG, "Khởi tạo I2C");
    i2c_init_bus();

    // MAX30102
    i2c_dev_t dev_max;
    memset(&dev_max, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(max30102_initDesc(&dev_max, I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO));
    ESP_ERROR_CHECK(max30102_readPartID(&dev_max));
    ESP_ERROR_CHECK(max30102_init(
        0x1F,
        MAX30102_SAMPLEAVG_4,
        MAX30102_MODE_REDIRONLY,
        MAX30102_SAMPLERATE_400,   // Đọc đúng 400Hz
        MAX30102_PULSEWIDTH_411,
        MAX30102_ADCRANGE_4096,
        &record,
        &dev_max
    ));
    max30102_clearFIFO(&dev_max);

    // OLED SSD1306
    oled._address = 0x3C;
    i2c_master_init(&oled, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, -1);
    i2c_device_add(&oled, I2C_MASTER_NUM, -1, 0x3C);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);
    ssd1306_show_buffer(&oled);

    // // SD Card
    // esp_vfs_fat_sdmmc_mount_config_t mount_config = MOUNT_CONFIG_DEFAULT();
    // sdmmc_card_t *sdcard = NULL;
    // sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    // spi_bus_config_t bus_config = SPI_BUS_CONFIG_DEFAULT();
    // sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();

    // if (sdcard_initialize(&mount_config, &sdcard, &host, &bus_config, &slot_config) != ESP_OK) {
    //     ESP_LOGE(TAG, "Không thể khởi tạo SD card");
    // } else {
    //     sdcard_writeDataToFile("spo2_log", "RED,IR\n");
    // }

    // Vòng lặp chính
    while (1) {
        max30102_check(&record, &dev_max);

        while (max30102_available(&record)) {
            uint32_t red = max30102_getFIFORed(&record);
            uint32_t ir  = max30102_getFIFOIR(&record);

            // Ghi dữ liệu ra SD
            sdcard_writeDataToFile("spo2_log", "%lu,%lu\n", red, ir);

            // Cập nhật buffer trung bình trượt
            ir_buffer[ir_index] = ir;
            ir_index = (ir_index + 1) % MA_WINDOW_SIZE;

            // Mỗi 5 mẫu (~100Hz), hiển thị 1 điểm
            display_counter++;
            if (display_counter >= 5) {
                display_counter = 0;

                uint32_t ir_avg = moving_average(ir_buffer, MA_WINDOW_SIZE);
                int y = map_value(ir_avg, 20000, 100000, 63, 0);

                _ssd1306_pixel(&oled, x_pos, y, false);
                ssd1306_show_buffer(&oled);

                x_pos++;
                if (x_pos >= 128) {
                    x_pos = 0;
                    ssd1306_clear_screen(&oled, false);
                }
            }

            max30102_nextSample(&record);
        }

        vTaskDelay(pdMS_TO_TICKS(1));  // Đợi nhẹ để không chiếm CPU
    }

    vTaskDelete(NULL);
}

void app_main(void) {
    xTaskCreatePinnedToCore(max30102_main, "max30102_main", 4096 * 4, NULL, 5, NULL, 1);
}