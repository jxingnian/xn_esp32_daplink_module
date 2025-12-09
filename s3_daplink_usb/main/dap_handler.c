/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-08 21:23:12
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-09 14:45:46
 * @FilePath: \todo-xn_esp32_daplink_module\s3_daplink_usb\main\dap_handler.c
 * @Description: DAP 命令处理模块 - 负责处理来自 USB 主机的 CMSIS-DAP 命令
 * 
 * 本文件实现了 DAP 命令处理的核心逻辑：
 * 1. 从 USB Vendor 类端点接收 DAP 命令包
 * 2. 调用 CMSIS-DAP 协议栈处理命令
 * 3. 将响应数据发送回 USB 主机
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tusb.h"
#include "DAP_config.h"
#include "DAP.h"

/* 日志标签，用于 ESP_LOG 系列函数 */
static const char *TAG = "DAP_HANDLER";

/* ==================== DAP 数据包缓冲区 ==================== */

/**
 * @brief DAP 请求缓冲区
 * 
 * 用于存储从 USB 主机接收到的 DAP 命令数据包
 * 缓冲区大小由 DAP_PACKET_SIZE 定义（通常为 64 字节）
 */
static uint8_t dap_request[DAP_PACKET_SIZE];

/**
 * @brief DAP 响应缓冲区
 * 
 * 用于存储 DAP 命令处理后的响应数据包
 * 响应数据将通过 USB 发送回主机
 */
static uint8_t dap_response[DAP_PACKET_SIZE];

/* ==================== DAP 处理任务 ==================== */

/**
 * @brief DAP 命令处理任务
 * 
 * 该任务运行在独立的 FreeRTOS 任务中，负责：
 * 1. 初始化 DAP 硬件接口
 * 2. 循环检查 USB Vendor 端点是否有数据
 * 3. 接收并处理 DAP 命令
 * 4. 发送响应数据
 * 
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 该任务被固定在 Core 0 上运行，以确保稳定的时序
 * @note 任务优先级设置为 5，属于较高优先级
 */
static void dap_handler_task(void *pvParameters)
{
    ESP_LOGI(TAG, "DAP 处理任务已启动");

    /* 初始化 DAP 硬件接口（GPIO、SWD/JTAG 引脚等）*/
    DAP_Setup();

    /* 主循环：持续处理来自 USB 主机的 DAP 命令 */
    while (1) {
        /* 检查 USB Vendor 端点是否有可用数据 */
        if (tud_vendor_available()) {
            /* 从 USB 端点读取数据到请求缓冲区 */
            uint32_t count = tud_vendor_read(dap_request, sizeof(dap_request));
            
            if (count > 0) {
                /* 调试日志：正式使用时注释掉以提高性能 */
                // ESP_LOGI(TAG, "DAP CMD: 0x%02X, len=%lu", dap_request[0], count);

                /* 
                 * 调用 CMSIS-DAP 协议栈处理命令
                 * DAP_ProcessCommand() 会解析命令并执行相应操作
                 * 返回值为响应数据的长度
                 */
                uint32_t response_len = DAP_ProcessCommand(dap_request, dap_response);

                /* 如果有响应数据，发送回 USB 主机 */
                if (response_len > 0) {
                    /* 写入响应数据到 USB 发送缓冲区 */
                    uint32_t written = tud_vendor_write(dap_response, response_len);
                    
                    /* 刷新缓冲区，确保数据立即发送 */
                    tud_vendor_flush();
                    
                    /* 检查响应状态 (仅对 Transfer 命令) - 调试时取消注释 */
                    // if (dap_request[0] == 0x05 || dap_request[0] == 0x06) {
                    //     if (dap_response[1] != 0x01) {  // ACK != OK
                    //         ESP_LOGW(TAG, "Transfer ACK=0x%02X", dap_response[1]);
                    //     }
                    // }
                } else {
                    ESP_LOGE(TAG, "No response for CMD 0x%02X", dap_request[0]);
                }
            }
        }

        /* 
         * 仅在没有数据时短暂让出 CPU
         * 有数据时不延时，保证最大吞吐量
         */
        if (!tud_vendor_available()) {
            taskYIELD();  // 让出 CPU 但不阻塞
        }
    }
}

/* ==================== 公共接口函数 ==================== */

/**
 * @brief 初始化 DAP 处理模块
 * 
 * 该函数创建 DAP 命令处理任务，应在 USB 初始化完成后调用
 * 
 * 任务配置：
 * - 任务名称: "dap_handler"
 * - 堆栈大小: 4096 字节
 * - 任务优先级: 5（较高优先级，确保及时响应）
 * - 运行核心: Core 0（固定核心，避免任务迁移带来的时序问题）
 */
void dap_handler_init(void)
{
    ESP_LOGI(TAG, "正在初始化 DAP 处理模块...");

    /* 创建 DAP 处理任务并固定到 Core 0 */
    xTaskCreatePinnedToCore(
        dap_handler_task,
        "dap_handler",
        4096,
        NULL,
        5,
        NULL,
        1  // Run on core 0
    );
}
