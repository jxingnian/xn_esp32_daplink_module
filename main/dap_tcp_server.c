/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-10
 * @Description: DAP TCP服务器实现 - 网络DAP命令转发
 */

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "DAP.h"
#include "dap_tcp_server.h"

static const char *TAG = "DAP_TCP";

// 全局变量
static int g_server_sock = -1;
static bool g_is_running = false;
static TaskHandle_t g_server_task = NULL;

/**
 * @brief 处理单个客户端连接
 */
static void handle_client(int client_sock)
{
    ESP_LOGI(TAG, "✅ 客户端已连接");
    
    uint8_t request[64];
    uint8_t response[64];
    
    while (g_is_running) {
        // 接收DAP命令
        int len = recv(client_sock, request, sizeof(request), 0);
        if (len <= 0) {
            ESP_LOGW(TAG, "客户端断开连接");
            break;
        }
        
        ESP_LOGD(TAG, "收到DAP命令: %d字节", len);
        
        // 调用DAP处理函数
        uint32_t resp_len = DAP_ProcessCommand(request, response);
        
        // 发送响应
        if (resp_len > 0) {
            int sent = send(client_sock, response, resp_len, 0);
            if (sent < 0) {
                ESP_LOGE(TAG, "发送响应失败");
                break;
            }
            ESP_LOGD(TAG, "发送DAP响应: %lu字节", resp_len);
        }
    }
    
    close(client_sock);
    ESP_LOGI(TAG, "❌ 客户端连接关闭");
}

/**
 * @brief DAP TCP服务器任务
 */
static void dap_tcp_server_task(void *arg)
{
    uint16_t port = (uint16_t)(uintptr_t)arg;
    
    // 创建socket
    g_server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_sock < 0) {
        ESP_LOGE(TAG, "创建socket失败");
        g_is_running = false;
        vTaskDelete(NULL);
        return;
    }
    
    // 设置socket选项
    int opt = 1;
    setsockopt(g_server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(g_server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "绑定端口失败: %d", port);
        close(g_server_sock);
        g_server_sock = -1;
        g_is_running = false;
        vTaskDelete(NULL);
        return;
    }
    
    // 监听
    if (listen(g_server_sock, 1) < 0) {
        ESP_LOGE(TAG, "监听失败");
        close(g_server_sock);
        g_server_sock = -1;
        g_is_running = false;
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "✅ DAP TCP服务器已启动，监听端口: %d", port);
    
    // 接受连接
    while (g_is_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        // 设置超时，避免阻塞
        struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
        setsockopt(g_server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        int client_sock = accept(g_server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 超时，继续循环
                continue;
            }
            ESP_LOGE(TAG, "接受连接失败: %d", errno);
            continue;
        }
        
        // 处理客户端（阻塞式，一次只处理一个客户端）
        handle_client(client_sock);
    }
    
    close(g_server_sock);
    g_server_sock = -1;
    ESP_LOGI(TAG, "DAP TCP服务器已停止");
    
    vTaskDelete(NULL);
}

/**
 * @brief 启动DAP TCP服务器
 */
esp_err_t dap_tcp_server_start(uint16_t port)
{
    if (g_is_running) {
        ESP_LOGW(TAG, "DAP TCP服务器已在运行");
        return ESP_OK;
    }
    
    g_is_running = true;
    
    xTaskCreate(dap_tcp_server_task, "dap_tcp_server", 8192, 
                (void *)(uintptr_t)port, 5, &g_server_task);
    
    ESP_LOGI(TAG, "🚀 DAP TCP服务器启动中...");
    
    return ESP_OK;
}

/**
 * @brief 停止DAP TCP服务器
 */
esp_err_t dap_tcp_server_stop(void)
{
    g_is_running = false;
    
    if (g_server_sock >= 0) {
        shutdown(g_server_sock, SHUT_RDWR);
        close(g_server_sock);
        g_server_sock = -1;
    }
    
    ESP_LOGI(TAG, "DAP TCP服务器已停止");
    
    return ESP_OK;
}

/**
 * @brief 获取服务器运行状态
 */
bool dap_tcp_server_is_running(void)
{
    return g_is_running;
}
