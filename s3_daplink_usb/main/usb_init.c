#include "usb_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

static const char *TAG = "USB_INIT";

int usb_init(void)
{
    ESP_LOGI(TAG, "Initializing TinyUSB (default config)...");

    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return -1;
    }

    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    return 0;
}
