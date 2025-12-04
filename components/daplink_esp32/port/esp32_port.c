/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-04 10:41:55
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-04 12:18:21
 * @FilePath: \DAPLinkf:\code\xn_esp32_compoents\xn_esp32_daplink_module\components\daplink_esp32\port\esp32_port.c
 * @Description: ESP32-S3 系统接口实现
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
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
