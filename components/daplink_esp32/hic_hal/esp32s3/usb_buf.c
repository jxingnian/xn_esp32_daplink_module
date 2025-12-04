/**
 * @file    usb_buf.c
 * @brief   ESP32-S3 USB 缓冲区管理实现
 * 
 * @author  星年
 * @date    2025-12-04
 */

#include "esp32_hal.h"
#include "daplink_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "USB_BUF";

// USB 数据包结构
typedef struct {
    uint8_t data[DAP_PACKET_SIZE];
    uint32_t len;
} usb_packet_t;

// USB 发送和接收队列
static QueueHandle_t tx_queue = NULL;
static QueueHandle_t rx_queue = NULL;

/**
 * @brief USB 缓冲区初始化
 */
int usb_buf_init(void)
{
    ESP_LOGI(TAG, "Initializing USB buffers...");
    
    // 创建发送队列
    tx_queue = xQueueCreate(DAP_PACKET_COUNT, sizeof(usb_packet_t));
    if (tx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create TX queue");
        return -1;
    }
    
    // 创建接收队列
    rx_queue = xQueueCreate(DAP_PACKET_COUNT, sizeof(usb_packet_t));
    if (rx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create RX queue");
        vQueueDelete(tx_queue);
        return -1;
    }
    
    ESP_LOGI(TAG, "USB buffers initialized (packet size: %d, count: %d)",
             DAP_PACKET_SIZE, DAP_PACKET_COUNT);
    return 0;
}

/**
 * @brief 写入 USB 发送缓冲区
 */
int usb_buf_write(const uint8_t *data, uint32_t len)
{
    if (tx_queue == NULL || data == NULL || len == 0 || len > DAP_PACKET_SIZE) {
        return -1;
    }
    
    usb_packet_t packet;
    memcpy(packet.data, data, len);
    packet.len = len;
    
    if (xQueueSend(tx_queue, &packet, 0) != pdTRUE) {
        ESP_LOGW(TAG, "TX queue full, packet dropped");
        return -1;
    }
    
    return len;
}

/**
 * @brief 从 USB 接收缓冲区读取
 */
int usb_buf_read(uint8_t *data, uint32_t len)
{
    if (rx_queue == NULL || data == NULL || len == 0) {
        return -1;
    }
    
    usb_packet_t packet;
    
    if (xQueueReceive(rx_queue, &packet, 0) != pdTRUE) {
        return 0;  // 队列为空
    }
    
    uint32_t copy_len = (packet.len < len) ? packet.len : len;
    memcpy(data, packet.data, copy_len);
    
    return copy_len;
}

/**
 * @brief 获取发送队列句柄（供 USB 任务使用）
 */
QueueHandle_t usb_buf_get_tx_queue(void)
{
    return tx_queue;
}

/**
 * @brief 获取接收队列句柄（供 USB 任务使用）
 */
QueueHandle_t usb_buf_get_rx_queue(void)
{
    return rx_queue;
}
