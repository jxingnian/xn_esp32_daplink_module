/**
 * @file dap_handler.c
 * @brief CMSIS-DAP v2 命令处理（USB Bulk 传输）
 * @author 星年
 * @date 2025-12-07
 * 
 * 本文件实现了 CMSIS-DAP v2 协议的核心命令处理逻辑，支持通过 USB Bulk 传输
 * 与调试主机通信。主要功能包括：
 * - 设备信息查询 (DAP_Info)
 * - SWD 接口连接/断开 (DAP_Connect/DAP_Disconnect)
 * - SWD 传输配置 (DAP_TransferConfigure)
 * - SWD 数据传输 (DAP_Transfer)
 * - SWJ 序列发送 (DAP_SWJ_Sequence)
 * - SWD 接口配置 (DAP_SWD_Configure)
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

// USB 数据包配置 - 受 Full Speed USB 限制
#define DAP_PACKET_SIZE 64   // 单个数据包最大尺寸（字节）
#define DAP_PACKET_COUNT 4   // 缓冲区数据包数量

// CMSIS-DAP 命令标识符定义
#define ID_DAP_Info                 0x00  // 查询设备信息
#define ID_DAP_HostStatus           0x01  // 主机状态控制（LED等）
#define ID_DAP_Connect              0x02  // 连接到调试接口
#define ID_DAP_Disconnect           0x03  // 断开调试接口
#define ID_DAP_TransferConfigure    0x04  // 配置传输参数
#define ID_DAP_Transfer             0x05  // 执行数据传输
#define ID_DAP_TransferBlock        0x06  // 块传输（未实现）
#define ID_DAP_WriteABORT           0x08  // 写入 ABORT 寄存器
#define ID_DAP_Delay                0x09  // 延时命令
#define ID_DAP_ResetTarget          0x0A  // 复位目标设备
#define ID_DAP_SWJ_Pins             0x10  // 控制 SWJ 引脚
#define ID_DAP_SWJ_Clock            0x11  // 设置 SWJ 时钟频率
#define ID_DAP_SWJ_Sequence         0x12  // 发送 SWJ 序列
#define ID_DAP_SWD_Configure        0x13  // 配置 SWD 接口
#define ID_DAP_SWD_Sequence         0x1D  // 发送 SWD 序列

// DAP_Info 命令的信息类型标识符
#define DAP_ID_VENDOR               0x01  // 厂商名称
#define DAP_ID_PRODUCT              0x02  // 产品名称
#define DAP_ID_SER_NUM              0x03  // 序列号
#define DAP_ID_FW_VER               0x04  // 固件版本
#define DAP_ID_DEVICE_VENDOR        0x05  // 目标设备厂商
#define DAP_ID_DEVICE_NAME          0x06  // 目标设备名称
#define DAP_ID_CAPABILITIES         0xF0  // 设备能力标志
#define DAP_ID_PACKET_SIZE          0xFE  // 数据包大小
#define DAP_ID_PACKET_COUNT         0xFF  // 数据包数量

// DAP Transfer 请求字节位域定义
#define DAP_TRANSFER_APnDP        (1 << 0)  // 0=DP寄存器, 1=AP寄存器
#define DAP_TRANSFER_RnW          (1 << 1)  // 0=写操作, 1=读操作
#define DAP_TRANSFER_A2           (1 << 2)  // 地址位2
#define DAP_TRANSFER_A3           (1 << 3)  // 地址位3

// DAP Transfer 响应状态定义
#define DAP_TRANSFER_OK           (1 << 0)  // 传输成功
#define DAP_TRANSFER_WAIT         (1 << 1)  // 等待状态
#define DAP_TRANSFER_FAULT        (1 << 2)  // 故障状态
#define DAP_TRANSFER_ERROR        (1 << 3)  // 协议错误

// 全局状态变量
static uint8_t dap_connected = 0;    // 连接状态：0=未连接, 1=已连接
static uint8_t dap_port = 0;         // 调试端口：0=未连接, 1=SWD, 2=JTAG
static uint32_t idle_cycles = 0;     // SWD 传输间的空闲周期数
static uint32_t retry_count = 100;   // 传输重试次数

/**
 * @brief 处理 DAP_Info 命令 - 查询设备信息
 * @param request 输入命令数据
 * @param response 输出响应数据
 * @return 响应数据长度
 * 
 * 根据请求的信息类型ID，返回相应的设备信息：
 * - 厂商/产品名称、序列号、固件版本
 * - 设备能力标志（支持的调试接口和功能）
 * - USB 数据包配置参数
 */
static uint32_t dap_info(const uint8_t *request, uint8_t *response) {
    uint8_t id = request[1];  // 获取信息类型ID
    
    ESP_LOGI(TAG, "DAP_Info: ID=0x%02X", id);
    
    response[0] = ID_DAP_Info;  // 响应命令ID
    
    switch (id) {
        case DAP_ID_VENDOR: {
            // 返回厂商名称
            const char *str = "XingNian";
            uint8_t len = strlen(str);
            response[1] = len;                    // 字符串长度
            memcpy(&response[2], str, len);       // 字符串内容
            return 2 + len;
        }
        
        case DAP_ID_PRODUCT: {
            // 返回产品名称
            const char *str = "ESP32-S3 CMSIS-DAP";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            return 2 + len;
        }
        
        case DAP_ID_SER_NUM: {
            // 返回设备序列号
            const char *str = "123456";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            return 2 + len;
        }
        
        case DAP_ID_FW_VER: {
            // 返回 CMSIS-DAP 协议版本
            const char *str = "2.1.0";
            uint8_t len = strlen(str);
            response[1] = len;
            memcpy(&response[2], str, len);
            ESP_LOGI(TAG, "FW Version: %s", str);
            return 2 + len;
        }
        
        case DAP_ID_CAPABILITIES: {
            // 返回设备能力标志位
            response[1] = 1;  // 数据长度
            // 各位定义：
            // Bit 0: 支持 SWD 接口
            // Bit 1: 支持 JTAG 接口  
            // Bit 2: 支持 SWO UART 输出
            // Bit 3: 支持 SWO 曼彻斯特编码输出
            // Bit 4: 支持原子命令
            // Bit 5: 支持时间戳
            // Bit 6: 支持 SWO 流输出
            // 注意: Bit 7 (DAP v2) 不在此处设置!
            response[2] = (1 << 0) | (1 << 4);  // 支持 SWD + 原子命令
            ESP_LOGI(TAG, "Capabilities: 0x%02X", response[2]);
            return 3;
        }
        
        case DAP_ID_DEVICE_VENDOR:
        case DAP_ID_DEVICE_NAME: {
            // 目标设备信息（暂未实现，返回空字符串）
            response[1] = 0;  // 空字符串
            return 2;
        }
        
        case DAP_ID_PACKET_SIZE: {
            // 返回 USB 数据包最大尺寸（小端序）
            response[1] = 2;                                    // 数据长度
            response[2] = (DAP_PACKET_SIZE >> 0) & 0xFF;       // 低字节
            response[3] = (DAP_PACKET_SIZE >> 8) & 0xFF;       // 高字节
            ESP_LOGI(TAG, "Packet Size: %d", DAP_PACKET_SIZE);
            return 4;
        }
        
        case DAP_ID_PACKET_COUNT: {
            // 返回数据包缓冲区数量
            response[1] = 1;                    // 数据长度
            response[2] = DAP_PACKET_COUNT;     // 缓冲区数量
            ESP_LOGI(TAG, "Packet Count: %d", DAP_PACKET_COUNT);
            return 3;
        }
        
        default:
            // 未知的信息类型ID
            ESP_LOGW(TAG, "Unknown Info ID: 0x%02X", id);
            response[1] = 0;  // 返回空数据
            return 2;
    }
}

/**
 * @brief 处理 DAP_Connect 命令 - 连接到调试接口
 * @param request 输入命令数据
 * @param response 输出响应数据  
 * @return 响应数据长度
 * 
 * 根据请求的端口类型连接到相应的调试接口：
 * - 0: 自动检测（默认使用SWD）
 * - 1: SWD 接口
 * - 2: JTAG 接口（暂未支持）
 */
static uint32_t dap_connect(const uint8_t *request, uint8_t *response) {
    uint8_t port = request[1];  // 获取请求的端口类型
    
    response[0] = ID_DAP_Connect;  // 响应命令ID
    
    if (port == 1) {  // 请求连接 SWD 接口
        // 初始化 SWD 硬件接口
        PORT_SWD_SETUP();
        PORT_SWJ_CONNECT(1);  // 调用 HAL 层连接函数
        
        // 更新连接状态
        dap_connected = 1;
        dap_port = 1;
        response[1] = 1;  // 返回成功连接的端口类型
        ESP_LOGI(TAG, "Connected to SWD");
    } else if (port == 0) {  // 自动检测端口类型
        // 默认尝试 SWD 连接
        PORT_SWD_SETUP();
        PORT_SWJ_CONNECT(1);
        
        dap_connected = 1;
        dap_port = 1;
        response[1] = 1;  // 返回检测到的端口类型
        ESP_LOGI(TAG, "Auto-detected SWD");
    } else {
        // 不支持的端口类型
        response[1] = 0;  // 连接失败
        ESP_LOGI(TAG, "Connect failed: unsupported port %d", port);
    }
    
    return 2;
}

/**
 * @brief 处理 DAP_Disconnect 命令 - 断开调试接口
 * @param request 输入命令数据
 * @param response 输出响应数据
 * @return 响应数据长度
 * 
 * 断开当前的调试接口连接，释放硬件资源
 */
static uint32_t dap_disconnect(const uint8_t *request, uint8_t *response) {
    response[0] = ID_DAP_Disconnect;  // 响应命令ID
    response[1] = 0x00;               // 操作状态：0x00=成功
    
    // 调用 HAL 层断开连接
    PORT_SWJ_DISCONNECT();
    
    // 清除连接状态
    dap_connected = 0;
    dap_port = 0;
    ESP_LOGI(TAG, "Disconnected");
    return 2;
}

/**
 * @brief 处理 DAP_SWJ_Sequence 命令 - 发送 SWJ 序列
 * @param request 输入命令数据
 * @param response 输出响应数据
 * @return 响应数据长度
 * 
 * 在 SWD/JTAG 线路上发送指定的位序列，用于：
 * - 线路复位序列
 * - 协议切换序列  
 * - 自定义调试序列
 */
static uint32_t dap_swj_sequence(const uint8_t *request, uint8_t *response) {
    uint8_t count = request[1];  // 获取序列长度（位数）
    
    response[0] = ID_DAP_SWJ_Sequence;  // 响应命令ID
    
    // 特殊处理：0 表示 256 位
    if (count == 0) {
        count = 255;  // 实际发送 256 位
    }
    
    // 调用底层函数发送位序列
    // request[2] 开始是序列数据（按字节打包）
    SWJ_Sequence(count, &request[2]);
    
    response[1] = 0x00;  // 操作状态：0x00=成功
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
 * @brief 处理 DAP_Transfer 命令 - 执行调试端口传输操作
 * @param request 输入命令数据
 * @param response 输出响应数据
 * @return 响应数据长度
 * 
 * DAP_Transfer 是 CMSIS-DAP 协议中最重要的命令，用于执行对调试端口的读写操作。
 * 支持批量传输多个寄存器操作，提高调试效率。
 * 
 * 请求格式：
 * - request[0]: 命令ID (ID_DAP_Transfer)
 * - request[1]: DAP索引 (通常为0，多DAP设备时使用)
 * - request[2]: 传输数量 (1-255)
 * - request[3+]: 传输请求序列
 *   - 每个传输请求包含：
 *     - 1字节请求描述符 (APnDP, RnW, A2, A3位)
 *     - 写操作时跟随4字节数据 (小端序)
 * 
 * 响应格式：
 * - response[0]: 命令ID
 * - response[1]: 实际完成的传输数量
 * - response[2]: 最后一次传输的ACK响应
 * - response[3+]: 读操作的数据 (每个4字节，小端序)
 */
static uint32_t dap_transfer(const uint8_t *request, uint8_t *response) {
    // uint8_t dap_index = request[1];  // DAP 索引,暂未使用
    uint8_t transfer_count = request[2];  // 获取传输数量
    
    ESP_LOGI(TAG, "DAP_Transfer: count=%d", transfer_count);
    
    // 初始化响应头
    response[0] = ID_DAP_Transfer;     // 响应命令ID
    response[1] = transfer_count;      // 预设传输数量（错误时会更新为实际数量）
    response[2] = 0;                   // 响应值（最后一次 ACK）
    
    uint32_t req_idx = 3;   // 请求数据索引，从传输序列开始
    uint32_t resp_idx = 3;  // 响应数据索引，从数据区开始
    uint8_t ack = 0;        // 保存最后一次 ACK 响应
    
    // 逐个处理传输请求
    for (uint8_t i = 0; i < transfer_count; i++) {
        uint8_t request_byte = request[req_idx++];  // 获取传输请求描述符
        uint32_t data = 0;
        
        // 根据 RnW 位判断读写操作
        if (request_byte & DAP_TRANSFER_RnW) {
            // 读操作：从调试端口读取数据
            ESP_LOGI(TAG, "  [%d] READ req=0x%02X", i, request_byte);
            ack = SWD_Transfer(request_byte, &data);
            ESP_LOGI(TAG, "  [%d] ACK=%d, data=0x%08lX", i, ack, data);
            
            if (ack == DAP_TRANSFER_OK) {
                // 传输成功，将读取的数据写入响应（小端序）
                response[resp_idx++] = (data >> 0) & 0xFF;   // 字节0
                response[resp_idx++] = (data >> 8) & 0xFF;   // 字节1
                response[resp_idx++] = (data >> 16) & 0xFF;  // 字节2
                response[resp_idx++] = (data >> 24) & 0xFF;  // 字节3
            } else {
                // 传输失败，提前结束并返回错误信息
                response[1] = i;    // 更新实际传输数
                response[2] = ack;  // 保存错误ACK
                return resp_idx;
            }
        } else {
            // 写操作：向调试端口写入数据
            // 从请求中提取4字节数据（小端序）
            data = request[req_idx] | (request[req_idx+1] << 8) | 
                   (request[req_idx+2] << 16) | (request[req_idx+3] << 24);
            req_idx += 4;  // 跳过数据字节
            
            ESP_LOGI(TAG, "  [%d] WRITE req=0x%02X, data=0x%08lX", i, request_byte, data);
            ack = SWD_Transfer(request_byte, &data);
            ESP_LOGI(TAG, "  [%d] ACK=%d", i, ack);
            
            if (ack != DAP_TRANSFER_OK) {
                // 传输失败，提前结束并返回错误信息
                response[1] = i;    // 更新实际传输数
                response[2] = ack;  // 保存错误ACK
                return resp_idx;
            }
        }
    }
    
    // 所有传输成功完成
    response[2] = ack;  // 保存最后一次 ACK
    return resp_idx;
}

/**
 * @brief 处理 DAP 命令 - 命令分发器
 * @param request 输入命令数据
 * @param response 输出响应数据
 * @return 响应数据长度
 * 
 * 根据命令ID分发到相应的处理函数。支持的命令包括：
 * - 信息查询命令 (DAP_Info)
 * - 连接管理命令 (DAP_Connect, DAP_Disconnect)
 * - 配置命令 (DAP_TransferConfigure, DAP_SWD_Configure)
 * - 传输命令 (DAP_Transfer)
 * - 序列命令 (DAP_SWJ_Sequence)
 * - 控制命令 (DAP_HostStatus, DAP_ResetTarget)
 */
static uint32_t dap_process_command(const uint8_t *request, uint8_t *response) {
    uint8_t cmd = request[0];  // 获取命令ID
    
    switch (cmd) {
        case ID_DAP_Info:
            // 查询设备信息（厂商、产品、版本等）
            return dap_info(request, response);
        
        case ID_DAP_Connect:
            // 连接到指定的调试接口（SWD/JTAG）
            return dap_connect(request, response);
        
        case ID_DAP_Disconnect:
            // 断开当前调试接口连接
            return dap_disconnect(request, response);
        
        case ID_DAP_TransferConfigure:
            // 配置传输参数（空闲周期、重试次数）
            return dap_transfer_configure(request, response);
        
        case ID_DAP_Transfer:
            // 执行调试端口传输操作
            return dap_transfer(request, response);
        
        case ID_DAP_SWJ_Sequence:
            // 发送SWD/JTAG序列（复位、切换等）
            return dap_swj_sequence(request, response);
        
        case ID_DAP_SWD_Configure:
            // 配置SWD接口参数（周转周期、数据相位）
            return dap_swd_configure(request, response);
        
        case ID_DAP_HostStatus:
            // 主机状态指示（通常用于LED控制）
            response[0] = ID_DAP_HostStatus;
            response[1] = 0x00;  // 操作状态：成功
            return 2;
        
        case ID_DAP_ResetTarget:
            // 复位目标MCU
            response[0] = ID_DAP_ResetTarget;
            response[1] = 0x00;  // 操作状态：成功
            response[2] = 0x00;  // 执行状态：正常
            
            // 执行硬件复位序列
            gpio_hal_set_reset(true);   // 拉低复位引脚
            vTaskDelay(pdMS_TO_TICKS(10));  // 保持10ms
            gpio_hal_set_reset(false);  // 释放复位引脚
            
            return 3;
        
        default:
            // 未实现或不支持的命令
            ESP_LOGW(TAG, "Unimplemented command: 0x%02X", cmd);
            response[0] = 0xFF;  // 错误响应
            return 1;
    }
}

/**
 * @brief DAP 主任务 - 处理USB通信和命令执行
 * @param pvParameters 任务参数（未使用）
 * 
 * 这是DAP处理器的主要工作任务，负责：
 * 1. 监控USB连接状态
 * 2. 从USB接收CMSIS-DAP命令
 * 3. 调用命令处理函数
 * 4. 将响应发送回主机
 * 
 * 任务运行在无限循环中，通过USB Vendor类接口与主机通信。
 * 使用轮询方式检查数据，避免阻塞其他任务。
 */
static void dap_task(void *pvParameters) {
    uint8_t request[DAP_PACKET_SIZE];   // 接收缓冲区
    uint8_t response[DAP_PACKET_SIZE];  // 发送缓冲区
    
    ESP_LOGI(TAG, "DAP task started, waiting for commands...");
    
    while (1) {
        // 检查USB Vendor接口是否已挂载和可用
        if (!tud_vendor_mounted()) {
            vTaskDelay(pdMS_TO_TICKS(100));  // USB未就绪，等待100ms
            continue;
        }
        
        // 从USB Bulk OUT端点读取命令数据
        uint32_t rx_size = tud_vendor_read(request, sizeof(request));
        
        if (rx_size > 0) {
            // 收到命令，记录日志
            ESP_LOGI(TAG, "Received command: 0x%02X, size: %lu", request[0], rx_size);
            
            // 分发并处理命令
            uint32_t resp_size = dap_process_command(request, response);
            
            // 将响应发送到USB Bulk IN端点
            if (resp_size > 0) {
                uint32_t sent = tud_vendor_write(response, resp_size);
                tud_vendor_flush();  // 强制发送缓冲区数据
                ESP_LOGI(TAG, "Sent response: 0x%02X, size: %lu", response[0], sent);
            }
        }
        
        // 短暂让出CPU时间，避免看门狗超时和任务饥饿
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 初始化 DAP 处理器
 * 
 * 创建DAP主任务，开始处理CMSIS-DAP协议通信。
 * 任务配置：
 * - 堆栈大小：4KB（足够处理命令和缓冲区）
 * - 优先级：5（中等优先级，平衡响应性和系统稳定性）
 * - 运行核心：自动分配
 */
void dap_handler_init(void) {
    ESP_LOGI(TAG, "Initializing DAP handler...");
    
    // 创建DAP处理任务
    xTaskCreate(dap_task, "dap_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "DAP handler initialized");
}
