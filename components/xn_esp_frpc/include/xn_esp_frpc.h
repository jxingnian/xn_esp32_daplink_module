/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-10
 * @Description: ESP32 FRP客户端组件 - 简化版，专用于DAP端口映射
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FRP客户端配置
 */
typedef struct {
    const char *server_addr;        ///< FRP服务器地址
    uint16_t server_port;           ///< FRP服务器端口（默认7000）
    const char *auth_token;         ///< 认证token
    const char *proxy_name;         ///< 代理名称
    uint16_t local_port;            ///< 本地端口（DAP服务端口）
    uint16_t remote_port;           ///< 远程端口
    uint32_t heartbeat_interval;   ///< 心跳间隔（秒）
} xn_frpc_config_t;

/**
 * @brief FRP客户端默认配置
 */
#define XN_FRPC_DEFAULT_CONFIG() {              \
    .server_addr = "frp.example.com",           \
    .server_port = 7000,                        \
    .auth_token = "",                           \
    .proxy_name = "esp32_dap",                  \
    .local_port = 5555,                         \
    .remote_port = 5555,                        \
    .heartbeat_interval = 30,                   \
}

/**
 * @brief 初始化FRP客户端
 * 
 * @param config 配置参数
 * @return ESP_OK 成功
 */
esp_err_t xn_frpc_init(const xn_frpc_config_t *config);

/**
 * @brief 启动FRP客户端
 * 
 * @return ESP_OK 成功
 */
esp_err_t xn_frpc_start(void);

/**
 * @brief 停止FRP客户端
 * 
 * @return ESP_OK 成功
 */
esp_err_t xn_frpc_stop(void);

/**
 * @brief 获取连接状态
 * 
 * @return true 已连接
 * @return false 未连接
 */
bool xn_frpc_is_connected(void);

#ifdef __cplusplus
}
#endif
