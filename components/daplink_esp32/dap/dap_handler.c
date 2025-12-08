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
#include "SW_DP.h"
#include "esp32_hal.h"

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

// DAP Transfer 定义
#define DAP_TRANSFER_APnDP        (1 << 0)
#define DAP_TRANSFER_RnW          (1 << 1)
#define DAP_TRANSFER_A2           (1 << 2)
#define DAP_TRANSFER_A3           (1 << 3)

#define DAP_TRANSFER_OK           (1 << 0)
#define DAP_TRANSFER_WAIT         (1 << 1)
#define DAP_TRANSFER_FAULT        (1 << 2)
#define DAP_TRANSFER_ERROR        (1 << 3)

// 连接状态
static uint8_t dap_connected = 0;
static uint8_t dap_port = 0;  // 0=未连接, 1=SWD, 2=JTAG
static uint32_t idle_cycles = 0;
static uint32_t retry_count = 100;

/**
 * @brief 处理 DAP_Info 命令
 */
static uint32_t dap_info(const uint8_t *request, uint8_t *response) {
    uint8_t id = request[1];
    
    ESP_LOGI(TAG, "DAP_Info: ID=0x%02X", id);
    
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
            const char *str = "2.1.0";  // CMSIS-DAP 版本
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            ESP_LOGI(TAG, "FW Version: %s", str);
            return 2 + len;
        }
        
        case DAP_ID_CAPABILITIES: {
            response[1] = 1;  // 长度
            // Bit 0: SWD
            // Bit 1: JTAG
            // Bit 2: SWO UART
            // Bit 3: SWO Manchester
            // Bit 4: Atomic Commands
            // Bit 5: Timestamp
            // Bit 6: SWO Streaming
            // 注意: Bit 7 (DAP v2) 不在这里设置!
            response[2] = (1 << 0) | (1 << 4);  // SWD + Atomic Commands
            ESP_LOGI(TAG, "Capabilities: 0x%02X", response[2]);
            return 3;
        }
        
        case DAP_ID_DEVICE_VENDOR:
        case DAP_ID_DEVICE_NAME: {
            response[1] = 0;  // 空字符串
            return 2;
        }
        
        case DAP_ID_PACKET_SIZE: {
            response[1] = 2;
            response[2] = (DAP_PACKET_SIZE >> 0) & 0xFF;
            response[3] = (DAP_PACKET_SIZE >> 8) & 0xFF;
            ESP_LOGI(TAG, "Packet Size: %d", DAP_PACKET_SIZE);
            return 4;
        }
        
        case DAP_ID_PACKET_COUNT: {
            response[1] = 1;
            response[2] = DAP_PACKET_COUNT;
            ESP_LOGI(TAG, "Packet Count: %d", DAP_PACKET_COUNT);
            return 3;
        }
        
        default:
            ESP_LOGW(TAG, "Unknown Info ID: 0x%02X", id);
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
        // 初始化 SWD 线路
        PORT_SWD_SETUP();
        PORT_SWJ_CONNECT(1);  // 调用 HAL 层连接函数
        
        dap_connected = 1;
        dap_port = 1;
        response[1] = 1;  // 连接成功
        ESP_LOGI(TAG, "Connected to SWD");
    } else if (port == 0) {  // 0 = 自动检测
        // 默认使用 SWD
        PORT_SWD_SETUP();
        PORT_SWJ_CONNECT(1);
        
        dap_connected = 1;
        dap_port = 1;
        response[1] = 1;
        ESP_LOGI(TAG, "Auto-detected SWD");
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
    
    PORT_SWJ_DISCONNECT();  // 调用 HAL 层断开函数
    
    dap_connected = 0;
    dap_port = 0;
    ESP_LOGI(TAG, "Disconnected");
    return 2;
}

/**
 * @brief 处理 DAP_SWJ_Sequence 命令
 */
static uint32_t dap_swj_sequence(const uint8_t *request, uint8_t *response) {
    uint8_t count = request[1];
    
    response[0] = ID_DAP_SWJ_Sequence;
    
    if (count == 0) {
        count = 255;  // 0 表示 256 位
    }
    
    // 发送序列
    SWJ_Sequence(count, &request[2]);
    
    response[1] = 0x00;  // OK
    return 2;
}

/**
 * @brief 处理 DAP_SWD_Configure 命令
 */
static uint32_t dap_swd_configure(const uint8_t *request, uint8_t *response) {
    uint8_t config = request[1];
    
    response[0] = ID_DAP_SWD_Configure;
    
    // config[1:0] = turnaround cycles (1-4)
    // config[2] = data phase
    uint8_t turnaround = (config & 0x03) + 1;
    uint8_t data_phase = (config >> 2) & 0x01;
    
    SWD_Configure(turnaround, data_phase);
    
    response[1] = 0x00;  // OK
    ESP_LOGI(TAG, "SWD Configure: turnaround=%d, data_phase=%d", turnaround, data_phase);
    return 2;
}

/**
 * @brief 处理 DAP_TransferConfigure 命令
 */
static uint32_t dap_transfer_configure(const uint8_t *request, uint8_t *response) {
    idle_cycles = request[1];
    retry_count = request[2] | (request[3] << 8);
    
    response[0] = ID_DAP_TransferConfigure;
    response[1] = 0x00;  // OK
    
    SWD_SetIdleCycles(idle_cycles);
    
    ESP_LOGI(TAG, "Transfer Configure: idle=%lu, retry=%lu", idle_cycles, retry_count);
    return 2;
}

/**
 * @brief 处理 DAP_Transfer 命令
 */
static uint32_t dap_transfer(const uint8_t *request, uint8_t *response) {
    // uint8_t dap_index = request[1];  // DAP 索引,暂未使用
    uint8_t transfer_count = request[2];
    
    ESP_LOGI(TAG, "DAP_Transfer: count=%d", transfer_count);
    
    response[0] = ID_DAP_Transfer;
    response[1] = transfer_count;  // 传输数量
    response[2] = 0;  // 响应值（最后一次 ACK）
    
    uint32_t req_idx = 3;
    uint32_t resp_idx = 3;
    uint8_t ack = 0;
    
    for (uint8_t i = 0; i < transfer_count; i++) {
        uint8_t request_byte = request[req_idx++];
        uint32_t data = 0;
        
        // 执行传输
        if (request_byte & DAP_TRANSFER_RnW) {
            // 读操作
            ESP_LOGI(TAG, "  [%d] READ req=0x%02X", i, request_byte);
            ack = SWD_Transfer(request_byte, &data);
            ESP_LOGI(TAG, "  [%d] ACK=%d, data=0x%08lX", i, ack, data);
            
            if (ack == DAP_TRANSFER_OK) {
                // 将数据写入响应
                response[resp_idx++] = (data >> 0) & 0xFF;
                response[resp_idx++] = (data >> 8) & 0xFF;
                response[resp_idx++] = (data >> 16) & 0xFF;
                response[resp_idx++] = (data >> 24) & 0xFF;
            } else {
                response[1] = i;  // 实际传输数
                response[2] = ack;
                return resp_idx;
            }
        } else {
            // 写操作
            data = request[req_idx] | (request[req_idx+1] << 8) | 
                   (request[req_idx+2] << 16) | (request[req_idx+3] << 24);
            req_idx += 4;
            
            ESP_LOGI(TAG, "  [%d] WRITE req=0x%02X, data=0x%08lX", i, request_byte, data);
            ack = SWD_Transfer(request_byte, &data);
            ESP_LOGI(TAG, "  [%d] ACK=%d", i, ack);
            
            if (ack != DAP_TRANSFER_OK) {
                response[1] = i;  // 实际传输数
                response[2] = ack;
                return resp_idx;
            }
        }
    }
    
    response[2] = ack;  // 最后一次 ACK
    return resp_idx;
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
        
        case ID_DAP_TransferConfigure:
            return dap_transfer_configure(request, response);
        
        case ID_DAP_Transfer:
            return dap_transfer(request, response);
        
        case ID_DAP_SWJ_Sequence:
            return dap_swj_sequence(request, response);
        
        case ID_DAP_SWD_Configure:
            return dap_swd_configure(request, response);
        
        case ID_DAP_HostStatus:
            // 占位：主机状态（LED 控制）
            response[0] = ID_DAP_HostStatus;
            response[1] = 0x00;  // OK
            return 2;
        
        case ID_DAP_ResetTarget:
            // 复位目标
            response[0] = ID_DAP_ResetTarget;
            response[1] = 0x00;  // OK
            response[2] = 0x00;  // 执行状态
            gpio_hal_set_reset(true);   // 进入复位
            vTaskDelay(pdMS_TO_TICKS(10));
            gpio_hal_set_reset(false);  // 释放复位
            return 3;
        
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
