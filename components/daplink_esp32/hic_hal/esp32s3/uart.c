/**
 * @file    uart.c
 * @brief   ESP32-S3 UART 硬件抽象层实现
 * 
 * @author  星年
 * @date    2025-12-04
 */

#include "esp32_hal.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "UART_HAL";

#define UART_BUF_SIZE   (1024)

/**
 * @brief UART 初始化
 */
int uart_hal_init(uint8_t uart_num, uint32_t baud_rate)
{
    ESP_LOGI(TAG, "Initializing UART%d at %lu baud", uart_num, baud_rate);
    
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t ret;
    
    // 配置 UART 参数
    ret = uart_param_config(uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config UART parameters");
        return -1;
    }
    
    // 设置 UART 引脚（使用默认引脚）
    ret = uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins");
        return -1;
    }
    
    // 安装 UART 驱动
    ret = uart_driver_install(uart_num, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver");
        return -1;
    }
    
    ESP_LOGI(TAG, "UART%d initialized successfully", uart_num);
    return 0;
}

/**
 * @brief UART 发送数据
 */
int uart_hal_write(uint8_t uart_num, const uint8_t *data, uint32_t len)
{
    int written = uart_write_bytes(uart_num, (const char *)data, len);
    return written;
}

/**
 * @brief UART 接收数据
 */
int uart_hal_read(uint8_t uart_num, uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    int read = uart_read_bytes(uart_num, data, len, pdMS_TO_TICKS(timeout_ms));
    return read;
}
