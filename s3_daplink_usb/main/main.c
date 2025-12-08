#include <stdio.h>
#include "esp_log.h"
#include "usb_init.h"

static const char *TAG = "S3_DAPLINK_USB";

void app_main(void)
{
    ESP_LOGI(TAG, "s3_daplink_usb: app_main start");

    if (usb_init() != 0) {
        ESP_LOGE(TAG, "usb_init failed");
        return;
    }

    ESP_LOGI(TAG, "USB initialized, waiting for host...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
