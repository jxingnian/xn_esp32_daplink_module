/**
 * @file    gpio.c
 * @brief   ESP32-S3 GPIO 硬件抽象层实现
 * 
 * @author  星年
 * @date    2025-12-04
 */

#include "esp32_hal.h"
#include "daplink_config.h"
#include "DAP_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GPIO_HAL";

/**
 * @brief GPIO 初始化
 */
int gpio_hal_init(void)
{
    ESP_LOGI(TAG, "Initializing GPIO...");
    
    esp_err_t ret;
    
    /* ==================== 配置 SWD 引脚 ==================== */
    
    // SWCLK - 输出，初始低电平
    gpio_config_t swclk_cfg = {
        .pin_bit_mask = (1ULL << PIN_SWCLK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&swclk_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config SWCLK");
        return -1;
    }
    gpio_set_level(PIN_SWCLK, 0);
    
    // SWDIO - 输出，初始高电平
    gpio_config_t swdio_cfg = {
        .pin_bit_mask = (1ULL << PIN_SWDIO),
        .mode = GPIO_MODE_INPUT_OUTPUT,  // 双向
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&swdio_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config SWDIO");
        return -1;
    }
    gpio_set_level(PIN_SWDIO, 1);
    
    /* ==================== 配置复位引脚 ==================== */
    
    // nRESET - 输出，初始高电平（释放复位）
    gpio_config_t reset_cfg = {
        .pin_bit_mask = (1ULL << PIN_nRESET),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&reset_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config nRESET");
        return -1;
    }
    gpio_set_level(PIN_nRESET, 1);
    
    /* ==================== 配置 LED 引脚 ==================== */
    
    // LED_CONNECTED - 输出，初始熄灭
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << PIN_LED_CONNECTED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&led_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config LED");
        return -1;
    }
    LED_CONNECTED_OFF();
    
#if ENABLE_JTAG
    /* ==================== 配置 JTAG 引脚 ==================== */
    
    // TCK - 输出
    gpio_config_t tck_cfg = {
        .pin_bit_mask = (1ULL << PIN_TCK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&tck_cfg);
    gpio_set_level(PIN_TCK, 0);
    
    // TMS - 输出
    gpio_config_t tms_cfg = {
        .pin_bit_mask = (1ULL << PIN_TMS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&tms_cfg);
    gpio_set_level(PIN_TMS, 1);
    
    // TDI - 输出
    gpio_config_t tdi_cfg = {
        .pin_bit_mask = (1ULL << PIN_TDI),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&tdi_cfg);
    gpio_set_level(PIN_TDI, 1);
    
    // TDO - 输入
    gpio_config_t tdo_cfg = {
        .pin_bit_mask = (1ULL << PIN_TDO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&tdo_cfg);
#endif
    
    ESP_LOGI(TAG, "GPIO initialized successfully");
    ESP_LOGI(TAG, "  SWCLK: GPIO%d", PIN_SWCLK);
    ESP_LOGI(TAG, "  SWDIO: GPIO%d", PIN_SWDIO);
    ESP_LOGI(TAG, "  nRESET: GPIO%d", PIN_nRESET);
    ESP_LOGI(TAG, "  LED: GPIO%d", PIN_LED_CONNECTED);
    
    return 0;
}

/**
 * @brief 设置 LED 状态
 */
void gpio_hal_set_led(uint8_t led_id, bool state)
{
    if (led_id == 0) {
        // 连接状态 LED
        if (state) {
            LED_CONNECTED_ON();
        } else {
            LED_CONNECTED_OFF();
        }
    }
}

/**
 * @brief LED 闪烁
 */
void gpio_hal_led_blink(uint8_t led_id, uint8_t count, uint32_t delay_ms)
{
    for (uint8_t i = 0; i < count; i++) {
        gpio_hal_set_led(led_id, true);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_hal_set_led(led_id, false);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * @brief 设置目标复位引脚状态
 */
void gpio_hal_set_reset(bool state)
{
    if (state) {
        // 进入复位（低电平）
        PIN_nRESET_CLR();
        ESP_LOGD(TAG, "Target RESET asserted");
    } else {
        // 释放复位（高电平）
        PIN_nRESET_SET();
        ESP_LOGD(TAG, "Target RESET released");
    }
}

/**
 * @brief 读取目标复位引脚状态
 */
bool gpio_hal_get_reset(void)
{
    return (PIN_nRESET_IN() == 0);  // 低电平表示复位中
}

/**
 * @brief DAP 端口初始化（供 CMSIS-DAP 调用）
 */
void PORT_DAP_SETUP(void)
{
    gpio_hal_init();
    ESP_LOGI(TAG, "DAP port setup completed");
}

/**
 * @brief 设置 DAP 时钟频率（供 CMSIS-DAP 调用）
 */
uint32_t PORT_SWJ_CLOCK_SET(uint32_t clock)
{
    // 限制时钟频率范围
    if (clock > DAP_MAX_SWJ_CLOCK) {
        clock = DAP_MAX_SWJ_CLOCK;
    } else if (clock < DAP_MIN_SWJ_CLOCK) {
        clock = DAP_MIN_SWJ_CLOCK;
    }
    
    ESP_LOGI(TAG, "SWJ clock set to %lu Hz", clock);
    return clock;
}

/**
 * @brief 连接到目标设备（供 CMSIS-DAP 调用）
 */
void PORT_SWJ_CONNECT(uint32_t port)
{
    if (port == 1) {
        // SWD 模式
        ESP_LOGI(TAG, "Connecting in SWD mode");
        
        // 配置 SWDIO 为输出
        PIN_SWDIO_OUT_ENABLE();
        
        // 释放复位
        gpio_hal_set_reset(false);
        
        // LED 指示连接
        gpio_hal_set_led(0, true);
        
    } else if (port == 2) {
        // JTAG 模式
        ESP_LOGI(TAG, "Connecting in JTAG mode");
        // TODO: JTAG 初始化
    }
}

/**
 * @brief 断开与目标设备的连接（供 CMSIS-DAP 调用）
 */
void PORT_SWJ_DISCONNECT(void)
{
    ESP_LOGI(TAG, "Disconnecting from target");
    
    // 所有引脚设置为输入（高阻态）
    gpio_set_direction(PIN_SWCLK, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT);
    
    // LED 熄灭
    gpio_hal_set_led(0, false);
}
