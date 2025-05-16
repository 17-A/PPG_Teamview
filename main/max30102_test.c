#include <stdio.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <freertos/ringbuf.h>

// Incluse MAX30102 driver
#include "max30102.h"
#include "ssd1306.h"
#include "string.h"

#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO           22          /*!< GPIO số cho SCL */
#define I2C_MASTER_SDA_IO           21          /*!< GPIO số cho SDA */
#define I2C_MASTER_NUM              I2C_NUM_0    /*!< Số I2C */
#define I2C_MASTER_FREQ_HZ          100000       /*!< Tốc độ I2C */
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0


TaskHandle_t readMAXTask_handle = NULL;
struct max30102_record record;
struct max30102_data data;
unsigned long red, ir;

/**
 * @brief Read data from MAX30102 and send to ring buffer
 * 
 * @param pvParameters 
 */
// void max30102_test(void *pvParameters)
// {
//     i2c_dev_t dev;
//     memset(&dev, 0, sizeof(i2c_dev_t));
//     ESP_ERROR_CHECK(max30102_initDesc(&dev, 0, 21, 22));
//     if(max30102_readPartID(&dev) == ESP_OK) 
//     {
//         ESP_LOGI(__func__, "Found MAX30102!");
//     }
//     else {
//         ESP_LOGE(__func__, "Not found MAX30102");
//     }

//     uint8_t powerLed = 0x1F; //Cuong do led, tieu thu 6.4mA
//     uint8_t sampleAverage = 4; 
//     uint8_t ledMode = 2;
//     int sampleRate = 500; //Tan so lay mau cao thi kich thuoc BUFFER_SIZE cung phai thay doi de co thoi gian thuat toan xu ly cac mau
//     int pulseWidth = 411; //Xung cang rong, dai thu duoc cang nhieu (18 bit)
//     int adcRange = 16384; //14 bit ADC tieu thu 65.2pA moi LSB

//     ESP_ERROR_CHECK(max30102_init(powerLed, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange, &record, &dev));
//     max30102_clearFIFO(&dev);   
//     while (1){
//         max30102_check(&record, &dev); //Check the sensor, read up to 3 samples
//         while (max30102_available(&record)) //do we have new data?
//         {
//             red = max30102_getFIFORed(&record);
//             ir = max30102_getFIFOIR(&record);
//             printf("%ld,%ld\n", red, ir);
//             max30102_nextSample(&record);
//         }
//     }
// }
int map_value(int x, int in_min, int in_max, int out_min, int out_max) {
    if (x < in_min) x = in_min;
    if (x > in_max) x = in_max;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void max30102_test(void *p) {
    struct max30102_record record;
    i2c_dev_t dev_max;

    // ✅ Khởi tạo cảm biến MAX30102 (đúng định dạng thư viện)
   max30102_init(
    0x1F,                       // ✅ powerLevel = 6.4mA
    MAX30102_SAMPLEAVG_4,
    MAX30102_MODE_REDIRONLY,
    MAX30102_SAMPLERATE_100,
    MAX30102_PULSEWIDTH_411,
    MAX30102_ADCRANGE_4096,
    &record,
    &dev_max
);


    // ✅ Khởi tạo OLED SSD1306
    SSD1306_t dev_oled;
    memset(&dev_oled, 0, sizeof(SSD1306_t));
    dev_oled._address = I2C_ADDRESS;     // 0x3C
    i2c_master_init(&dev_oled, 21, 22, -1); // SDA = 21, SCL = 22, RESET = -1
    i2c_device_add(&dev_oled, I2C_NUM_0, -1, I2C_ADDRESS);
    ssd1306_init(&dev_oled, 128, 64);

    ssd1306_clear_screen(&dev_oled, false);
    ssd1306_show_buffer(&dev_oled); // đúng hàm để cập nhật màn hình

    uint32_t ir = 0;
    int x_pos = 0;

    while (1) {
        // ✅ Đọc dữ liệu từ cảm biến
        max30102_check(&record, &dev_max);
        ir = max30102_getFIFOIR(&record);

        // ✅ Scale dữ liệu IR về pixel trên OLED (cao 64)
        int y_pos = map_value(ir, 20000, 100000, 63, 0);

        // ✅ Vẽ một pixel trên màn hình
        _ssd1306_pixel(&dev_oled, x_pos, y_pos, false);  // false = không đảo màu

        // ✅ Hiển thị buffer ra màn hình
        ssd1306_show_buffer(&dev_oled);

        // ✅ Tăng cột hiển thị
        x_pos++;
        if (x_pos >= 128) {
            x_pos = 0;
            ssd1306_clear_screen(&dev_oled, false);
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // Delay tương ứng ~100Hz
    }
}
void i2c_init_bus() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}




// void app_main(void){
//     // ESP_ERROR_CHECK(i2cdev_init()); 
//     i2c_master_init();  // thay vì i2cdev_init()
//     xTaskCreatePinnedToCore(max30102_test, "max30102_test", 1024 * 5, &readMAXTask_handle, 6, NULL, 0);
// }

void app_main(void)
{
    i2c_init_bus();  // ✅ tên mới, không bị trùng với hàm trong SSD1306
    xTaskCreatePinnedToCore(max30102_test, "max30102_test", 4096 * 2, NULL, 5, NULL, 1);
}
