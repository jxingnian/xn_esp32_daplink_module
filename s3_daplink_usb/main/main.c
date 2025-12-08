/**
 * @file main.c
 * @brief ESP32-S3 DAPLink USB 主程序入口
 * @author 星年
 * @date 2025-12-08
 * 
 * 本文件是 s3_daplink_usb 项目的主入口点，负责初始化 USB 设备和
 * CMSIS-DAP 处理器，使 ESP32-S3 成为一个功能完整的调试探针。
 * 
 * 系统启动流程：
 * 1. 初始化 USB 设备协议栈（TinyUSB）
 * 2. 启动 DAP 命令处理任务
 * 3. 进入主循环等待调试主机连接
 * 
 * Copyright (c) 2025 by 星年, All Rights Reserved.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "usb_init.h"
#include "dap_handler.h"

/* 日志标签 - 用于标识本模块的日志输出 */
static const char *TAG = "S3_DAPLINK_USB";

/**
 * @brief 应用程序主入口点
 * 
 * ESP-IDF 框架在完成系统初始化后自动调用此函数。
 * 本函数负责：
 * - 初始化 USB 外设和 TinyUSB 协议栈
 * - 启动 CMSIS-DAP 命令处理任务
 * - 维持主任务运行（防止函数返回导致任务销毁）
 * 
 * @note 此函数不应返回，否则 FreeRTOS 会删除主任务
 */
void app_main(void)
{
    /* 打印启动信息 */
    ESP_LOGI(TAG, "s3_daplink_usb: app_main start");

    /*
     * 步骤 1: 初始化 USB 设备
     * 
     * usb_init() 会配置 TinyUSB 协议栈，包括：
     * - 设置 USB 物理层参数
     * - 加载设备描述符、配置描述符
     * - 启动 TinyUSB 设备任务
     * 
     * 如果初始化失败，记录错误并退出
     */
    if (usb_init() != 0) {
        ESP_LOGE(TAG, "usb_init failed");
        return;
    }

    ESP_LOGI(TAG, "USB initialized, starting DAP handler...");

    /*
     * 步骤 2: 初始化 DAP 命令处理器
     * 
     * dap_handler_init() 会创建一个 FreeRTOS 任务，负责：
     * - 监听 USB Vendor 类接口的数据
     * - 解析 CMSIS-DAP 命令
     * - 通过 SWD 接口与目标芯片通信
     * - 将响应数据发送回主机
     */
    dap_handler_init();

    ESP_LOGI(TAG, "DAP handler started, waiting for host...");

    /*
     * 步骤 3: 主循环
     * 
     * 主任务进入空闲循环，定期让出 CPU 时间。
     * 实际的 DAP 处理工作由 dap_handler_task 完成。
     * 
     * 注意：此循环不能退出，否则 app_main 任务会被删除
     * 
     * 每隔 1 秒打印一次心跳信息（可选，用于调试）
     */
    while (1) {
        /* 延时 1 秒，让出 CPU 给其他任务 */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
