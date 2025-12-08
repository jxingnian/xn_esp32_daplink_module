#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "usb_init.h"
#include "dap_handler.h"

static const char *TAG = "S3_DAPLINK_USB";

void app_main(void)
{
    ESP_LOGI(TAG, "s3_daplink_usb: app_main start");

    if (usb_init() != 0) {
        ESP_LOGE(TAG, "usb_init failed");
        return;
    }

    ESP_LOGI(TAG, "USB initialized, starting DAP handler...");

    // Initialize DAP handler task
    dap_handler_init();

    ESP_LOGI(TAG, "DAP handler started, waiting for host...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
