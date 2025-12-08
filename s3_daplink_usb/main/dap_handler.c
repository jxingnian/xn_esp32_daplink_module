/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-08 21:23:12
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-08 21:29:27
 * @FilePath: \todo-xn_esp32_daplink_module\s3_daplink_usb\main\dap_handler.c
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
/**
 * @file dap_handler.c
 * @brief DAP command handler for USB Vendor class
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tusb.h"
#include "DAP_config.h"
#include "DAP.h"

static const char *TAG = "DAP_HANDLER";

// DAP packet buffers
static uint8_t dap_request[DAP_PACKET_SIZE];
static uint8_t dap_response[DAP_PACKET_SIZE];

// DAP handler task
static void dap_handler_task(void *pvParameters)
{
    ESP_LOGI(TAG, "DAP handler task started");

    // Initialize DAP
    DAP_Setup();

    while (1) {
        // Check if there's data available from USB host
        if (tud_vendor_available()) {
            uint32_t count = tud_vendor_read(dap_request, sizeof(dap_request));
            if (count > 0) {
                ESP_LOGD(TAG, "Received %lu bytes, cmd=0x%02X", count, dap_request[0]);

                // Process DAP command
                uint32_t response_len = DAP_ProcessCommand(dap_request, dap_response);

                // Send response back to host
                if (response_len > 0) {
                    tud_vendor_write(dap_response, response_len);
                    tud_vendor_flush();
                }
            }
        }

        // Small delay to prevent busy-waiting
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void dap_handler_init(void)
{
    ESP_LOGI(TAG, "Initializing DAP handler...");

    // Create DAP handler task
    xTaskCreatePinnedToCore(
        dap_handler_task,
        "dap_handler",
        4096,
        NULL,
        5,
        NULL,
        0  // Run on core 0
    );
}
