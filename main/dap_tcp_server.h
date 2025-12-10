/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-10
 * @Description: DAP TCP服务器 - 接收网络DAP命令并转发到本地DAP处理器
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动DAP TCP服务器
 * 
 * @param port 监听端口（默认5555）
 * @return ESP_OK 成功
 */
esp_err_t dap_tcp_server_start(uint16_t port);

/**
 * @brief 停止DAP TCP服务器
 * 
 * @return ESP_OK 成功
 */
esp_err_t dap_tcp_server_stop(void);

/**
 * @brief 获取服务器运行状态
 * 
 * @return true 运行中
 */
bool dap_tcp_server_is_running(void);

#ifdef __cplusplus
}
#endif
