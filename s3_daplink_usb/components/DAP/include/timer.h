/**
 * @file timer.h
 * @brief Timer utilities for DAP
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// Timestamp clock frequency (5 MHz)
#define TIMESTAMP_CLOCK 5000000U

// Get current timestamp (used by DAP_config.h TIMESTAMP_GET)
static inline uint32_t get_timer_count(void)
{
    return (uint32_t)(esp_timer_get_time() * 5);  // Convert us to 5MHz ticks
}

// Delay in milliseconds (used by DAP_config.h osDelay macro)
static inline void dap_os_delay(int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif // __TIMER_H__
