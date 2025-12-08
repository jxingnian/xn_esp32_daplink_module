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

// Get current timestamp in microseconds
static inline uint32_t dap_get_timestamp(void)
{
    return (uint32_t)(esp_timer_get_time() * 5);  // Convert us to 5MHz ticks
}

// Delay in milliseconds
static inline void dap_os_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif // __TIMER_H__
