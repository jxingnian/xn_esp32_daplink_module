/**
 * @file    esp32_port.c
 * @brief   ESP32-S3 系统接口实现
 * 
 * @author  星年
 * @date    2025-12-04
 */

#include "esp32_hal.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief 获取系统时间戳(微秒)
 */
uint64_t system_get_time_us(void)
{
    return esp_timer_get_time();
}

/**
 * @brief 延时(微秒)
 */
void system_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

/**
 * @brief 延时(毫秒)
 */
void system_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
