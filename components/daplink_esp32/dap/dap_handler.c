/**
 * @file dap_handler.c
 * @brief CMSIS-DAP v2 命令处理（USB Bulk 传输）
 * @author 星年
 * @date 2025-12-07
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tusb.h"
#include "dap_handler.h"

static const char *TAG = "DAP";

#define DAP_PACKET_SIZE 512
#define DAP_PACKET_COUNT 4

// DAP 命令 ID
#define ID_DAP_Info                 0x00
#define ID_DAP_HostStatus           0x01
#define ID_DAP_Connect              0x02
#define ID_DAP_Disconnect           0x03
#define ID_DAP_TransferConfigure    0x04
#define ID_DAP_Transfer             0x05
#define ID_DAP_TransferBlock        0x06
#define ID_DAP_WriteABORT           0x08
#define ID_DAP_Delay                0x09
#define ID_DAP_ResetTarget          0x0A
#define ID_DAP_SWJ_Pins             0x10
#define ID_DAP_SWJ_Clock            0x11
#define ID_DAP_SWJ_Sequence         0x12
#define ID_DAP_SWD_Configure        0x13
#define ID_DAP_SWD_Sequence         0x1D

// DAP Info ID
#define DAP_ID_VENDOR               0x01
#define DAP_ID_PRODUCT              0x02
#define DAP_ID_SER_NUM              0x03
#define DAP_ID_FW_VER               0x04
#define DAP_ID_DEVICE_VENDOR        0x05
#define DAP_ID_DEVICE_NAME          0x06
#define DAP_ID_CAPABILITIES         0xF0
#define DAP_ID_PACKET_SIZE          0xFE
#define DAP_ID_PACKET_COUNT         0xFF

// 连接状态
static uint8_t dap_connected = 0;

/**
 * @brief 处理 DAP_Info 命令
 */
static uint32_t dap_info(const uint8_t *request, uint8_t *response) {
    uint8_t id = request[1];
    
    response[0] = ID_DAP_Info;
    
    switch (id) {
        case DAP_ID_VENDOR: {
            const char *str = "XingNian";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            return 2 + len;
        }
        
        case DAP_ID_PRODUCT: {
            const char *str = "ESP32-S3 CMSIS-DAP";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            return 2 + len;
        }
        
        case DAP_ID_SER_NUM: {
            const char *str = "123456";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            return 2 + len;
        }
        
        case DAP_ID_FW_VER: {
            const char *str = "2.0.0";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            return 2 + len;
        }
        
        case DAP_ID_CAPABILITIES: {
            response[1] = 1;
            response[2] = 0x01;  // SWD 支持
            return 3;
        }
        
        case DAP_ID_PACKET_SIZE: {
            response[1] = 2;
            response[2] = (DAP_PACKET_SIZE >> 0) & 0xFF;
            response[3] = (DAP_PACKET_SIZE >> 8) & 0xFF;
            return 4;
        }
        
        case DAP_ID_PACKET_COUNT: {
            response[1] = 1;
            response[2] = DAP_PACKET_COUNT;
            return 3;
        }
        
        default:
            response[1] = 0;
            return 2;
    }
}

/**
 * @brief 处理 DAP_Connect 命令
 */
static uint32_t dap_connect(const uint8_t *request, uint8_t *response) {
    uint8_t port = request[1];
    
    response[0] = ID_DAP_Connect;
    
    if (port == 1) {  // 1 = SWD
        dap_connected = 1;
        response[1] = 1;  // 连接成功
        ESP_LOGI(TAG, "Connected to SWD");
    } else {
        response[1] = 0;  // 失败
        ESP_LOGI(TAG, "Connect failed: unsupported port %d", port);
    }
    
    return 2;
}

/**
 * @brief 处理 DAP_Disconnect 命令
 */
static uint32_t dap_disconnect(const uint8_t *request, uint8_t *response) {
    response[0] = ID_DAP_Disconnect;
    response[1] = 0x00;  // OK
    dap_connected = 0;
    ESP_LOGI(TAG, "Disconnected");
    return 2;
}

/**
 * @brief 处理 DAP 命令
 */
static uint32_t dap_process_command(const uint8_t *request, uint8_t *response) {
    uint8_t cmd = request[0];
    
    switch (cmd) {
        case ID_DAP_Info:
            return dap_info(request, response);
        
        case ID_DAP_Connect:
            return dap_connect(request, response);
        
        case ID_DAP_Disconnect:
            return dap_disconnect(request, response);
        
        case ID_DAP_HostStatus:
            // 占位：主机状态（LED 控制）
            response[0] = ID_DAP_HostStatus;
            response[1] = 0x00;  // OK
            return 2;
        
        default:
            // 未实现的命令
            ESP_LOGW(TAG, "Unimplemented command: 0x%02X", cmd);
            response[0] = 0xFF;  // 错误
            return 1;
    }
}

/**
 * @brief DAP 任务
 */
static void dap_task(void *pvParameters) {
    uint8_t request[DAP_PACKET_SIZE];
    uint8_t response[DAP_PACKET_SIZE];
    
    ESP_LOGI(TAG, "DAP task started, waiting for commands...");
    
    while (1) {
        // 等待 USB Vendor 接口可用
        if (!tud_vendor_mounted()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // 从 USB Bulk OUT 端点读取命令
        uint32_t rx_size = tud_vendor_read(request, sizeof(request));
        
        if (rx_size > 0) {
            ESP_LOGI(TAG, "Received command: 0x%02X, size: %lu", request[0], rx_size);
            
            // 处理命令
            uint32_t resp_size = dap_process_command(request, response);
            
            // 发送响应到 USB Bulk IN 端点
            if (resp_size > 0) {
                uint32_t sent = tud_vendor_write(response, resp_size);
                tud_vendor_flush();
                ESP_LOGI(TAG, "Sent response: 0x%02X, size: %lu", response[0], sent);
            }
        }
        
        // 让出 CPU，避免看门狗超时
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 初始化 DAP 处理器
 */
void dap_handler_init(void) {
    ESP_LOGI(TAG, "Initializing DAP handler...");
    
    // 创建 DAP 任务
    xTaskCreate(dap_task, "dap_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "DAP handler initialized");
}
