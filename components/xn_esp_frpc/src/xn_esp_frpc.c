/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-10
 * @Description: ESP32 FRP客户端实现 - 简化版TCP端口映射
 */

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "xn_esp_frpc.h"

static const char *TAG = "XN_FRPC";

// 全局变量
static xn_frpc_config_t g_config = {0};
static bool g_is_running = false;
static bool g_is_connected = false;
static int g_control_sock = -1;
static TaskHandle_t g_frpc_task = NULL;

// FRP协议消息类型
#define MSG_TYPE_LOGIN          'o'
#define MSG_TYPE_LOGIN_RESP     '1'
#define MSG_TYPE_NEW_PROXY      'p'
#define MSG_TYPE_NEW_PROXY_RESP '2'
#define MSG_TYPE_PING           'h'
#define MSG_TYPE_PONG           '4'
#define MSG_TYPE_REQ_WORK_CONN  'r'
#define MSG_TYPE_START_WORK_CONN 's'

/**
 * @brief 发送FRP消息
 */
static int frpc_send_msg(int sock, uint8_t type, const char *data, uint32_t len)
{
    if (sock < 0) {
        return -1;
    }
    
    // FRP消息格式: [type:1][length:4][data:length]
    uint8_t buf[5 + len];
    buf[0] = type;
    buf[1] = (len >> 24) & 0xFF;
    buf[2] = (len >> 16) & 0xFF;
    buf[3] = (len >> 8) & 0xFF;
    buf[4] = len & 0xFF;
    
    if (len > 0 && data) {
        memcpy(buf + 5, data, len);
    }
    
    int ret = send(sock, buf, 5 + len, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "发送消息失败: %d", errno);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 接收FRP消息
 */
static int frpc_recv_msg(int sock, uint8_t *type, char *data, uint32_t *len, uint32_t max_len)
{
    if (sock < 0) {
        return -1;
    }
    
    // 接收消息头
    uint8_t header[5];
    int ret = recv(sock, header, 5, MSG_WAITALL);
    if (ret != 5) {
        return -1;
    }
    
    *type = header[0];
    uint32_t msg_len = (header[1] << 24) | (header[2] << 16) | (header[3] << 8) | header[4];
    
    if (msg_len > max_len) {
        ESP_LOGE(TAG, "消息过长: %lu", msg_len);
        return -1;
    }
    
    // 接收消息体
    if (msg_len > 0) {
        ret = recv(sock, data, msg_len, MSG_WAITALL);
        if (ret != msg_len) {
            return -1;
        }
    }
    
    *len = msg_len;
    return 0;
}

/**
 * @brief 登录FRP服务器
 */
static int frpc_login(int sock)
{
    ESP_LOGI(TAG, "正在登录FRP服务器...");
    
    // 构造登录消息（简化版JSON）
    char login_msg[512];
    snprintf(login_msg, sizeof(login_msg),
             "{\"version\":\"0.43.0\","
             "\"hostname\":\"esp32-dap\","
             "\"os\":\"esp32\","
             "\"arch\":\"xtensa\","
             "\"user\":\"\","
             "\"timestamp\":%ld,"
             "\"privilege_key\":\"%s\","
             "\"run_id\":\"\","
             "\"pool_count\":1}",
             (long)time(NULL),
             g_config.auth_token ? g_config.auth_token : "");
    
    // 发送登录请求
    if (frpc_send_msg(sock, MSG_TYPE_LOGIN, login_msg, strlen(login_msg)) < 0) {
        ESP_LOGE(TAG, "发送登录请求失败");
        return -1;
    }
    
    // 接收登录响应
    uint8_t resp_type;
    char resp_data[1024];
    uint32_t resp_len;
    
    if (frpc_recv_msg(sock, &resp_type, resp_data, &resp_len, sizeof(resp_data)) < 0) {
        ESP_LOGE(TAG, "接收登录响应失败");
        return -1;
    }
    
    if (resp_type != MSG_TYPE_LOGIN_RESP) {
        ESP_LOGE(TAG, "登录响应类型错误: 0x%02X", resp_type);
        return -1;
    }
    
    ESP_LOGI(TAG, "✅ 登录成功");
    return 0;
}

/**
 * @brief 注册代理
 */
static int frpc_register_proxy(int sock)
{
    ESP_LOGI(TAG, "正在注册代理: %s", g_config.proxy_name);
    
    // 构造代理注册消息
    char proxy_msg[512];
    snprintf(proxy_msg, sizeof(proxy_msg),
             "{\"proxy_name\":\"%s\","
             "\"proxy_type\":\"tcp\","
             "\"use_encryption\":false,"
             "\"use_compression\":false,"
             "\"remote_port\":%d}",
             g_config.proxy_name,
             g_config.remote_port);
    
    // 发送代理注册请求
    if (frpc_send_msg(sock, MSG_TYPE_NEW_PROXY, proxy_msg, strlen(proxy_msg)) < 0) {
        ESP_LOGE(TAG, "发送代理注册请求失败");
        return -1;
    }
    
    // 接收代理注册响应
    uint8_t resp_type;
    char resp_data[1024];
    uint32_t resp_len;
    
    if (frpc_recv_msg(sock, &resp_type, resp_data, &resp_len, sizeof(resp_data)) < 0) {
        ESP_LOGE(TAG, "接收代理注册响应失败");
        return -1;
    }
    
    if (resp_type != MSG_TYPE_NEW_PROXY_RESP) {
        ESP_LOGE(TAG, "代理注册响应类型错误: 0x%02X", resp_type);
        return -1;
    }
    
    ESP_LOGI(TAG, "✅ 代理注册成功");
    return 0;
}

/**
 * @brief 心跳任务
 */
static void frpc_heartbeat_task(void *arg)
{
    while (g_is_running && g_is_connected) {
        vTaskDelay(pdMS_TO_TICKS(g_config.heartbeat_interval * 1000));
        
        if (g_control_sock >= 0) {
            if (frpc_send_msg(g_control_sock, MSG_TYPE_PING, NULL, 0) < 0) {
                ESP_LOGW(TAG, "心跳发送失败");
                g_is_connected = false;
                break;
            }
        }
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief 处理工作连接请求
 */
static void frpc_handle_work_conn(void)
{
    ESP_LOGI(TAG, "收到工作连接请求");
    
    // 创建新的socket连接到FRP服务器
    int work_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (work_sock < 0) {
        ESP_LOGE(TAG, "创建工作socket失败");
        return;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(g_config.server_port);
    
    struct hostent *server = gethostbyname(g_config.server_addr);
    if (server == NULL) {
        ESP_LOGE(TAG, "DNS解析失败");
        close(work_sock);
        return;
    }
    
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(work_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "工作连接失败");
        close(work_sock);
        return;
    }
    
    // 发送StartWorkConn消息
    if (frpc_send_msg(work_sock, MSG_TYPE_START_WORK_CONN, "{}", 2) < 0) {
        ESP_LOGE(TAG, "发送StartWorkConn失败");
        close(work_sock);
        return;
    }
    
    // 连接本地DAP服务
    int local_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (local_sock < 0) {
        ESP_LOGE(TAG, "创建本地socket失败");
        close(work_sock);
        return;
    }
    
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    local_addr.sin_port = htons(g_config.local_port);
    
    if (connect(local_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "连接本地服务失败");
        close(work_sock);
        close(local_sock);
        return;
    }
    
    ESP_LOGI(TAG, "✅ 工作连接建立");
    
    // 双向转发数据
    fd_set read_fds;
    uint8_t buffer[2048];
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(work_sock, &read_fds);
        FD_SET(local_sock, &read_fds);
        
        int max_fd = (work_sock > local_sock) ? work_sock : local_sock;
        
        struct timeval timeout = {.tv_sec = 30, .tv_usec = 0};
        int ret = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ret <= 0) {
            break;
        }
        
        // 从远程读取，发送到本地
        if (FD_ISSET(work_sock, &read_fds)) {
            int len = recv(work_sock, buffer, sizeof(buffer), 0);
            if (len <= 0) {
                break;
            }
            send(local_sock, buffer, len, 0);
        }
        
        // 从本地读取，发送到远程
        if (FD_ISSET(local_sock, &read_fds)) {
            int len = recv(local_sock, buffer, sizeof(buffer), 0);
            if (len <= 0) {
                break;
            }
            send(work_sock, buffer, len, 0);
        }
    }
    
    close(work_sock);
    close(local_sock);
    ESP_LOGI(TAG, "工作连接关闭");
}

/**
 * @brief FRP客户端主任务
 */
static void frpc_main_task(void *arg)
{
    while (g_is_running) {
        // 连接FRP服务器
        ESP_LOGI(TAG, "连接FRP服务器: %s:%d", g_config.server_addr, g_config.server_port);
        
        g_control_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (g_control_sock < 0) {
            ESP_LOGE(TAG, "创建socket失败");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(g_config.server_port);
        
        struct hostent *server = gethostbyname(g_config.server_addr);
        if (server == NULL) {
            ESP_LOGE(TAG, "DNS解析失败");
            close(g_control_sock);
            g_control_sock = -1;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        
        if (connect(g_control_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            ESP_LOGE(TAG, "连接失败: %d", errno);
            close(g_control_sock);
            g_control_sock = -1;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        ESP_LOGI(TAG, "✅ 已连接到FRP服务器");
        
        // 登录
        if (frpc_login(g_control_sock) < 0) {
            close(g_control_sock);
            g_control_sock = -1;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        // 注册代理
        if (frpc_register_proxy(g_control_sock) < 0) {
            close(g_control_sock);
            g_control_sock = -1;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        g_is_connected = true;
        
        // 启动心跳任务
        xTaskCreate(frpc_heartbeat_task, "frpc_heartbeat", 4096, NULL, 5, NULL);
        
        // 处理控制消息
        while (g_is_running && g_is_connected) {
            uint8_t msg_type;
            char msg_data[2048];
            uint32_t msg_len;
            
            int ret = frpc_recv_msg(g_control_sock, &msg_type, msg_data, &msg_len, sizeof(msg_data));
            if (ret < 0) {
                ESP_LOGW(TAG, "接收消息失败，断开连接");
                break;
            }
            
            switch (msg_type) {
                case MSG_TYPE_REQ_WORK_CONN:
                    frpc_handle_work_conn();
                    break;
                    
                case MSG_TYPE_PONG:
                    // 心跳响应
                    break;
                    
                default:
                    ESP_LOGD(TAG, "收到消息类型: 0x%02X", msg_type);
                    break;
            }
        }
        
        g_is_connected = false;
        close(g_control_sock);
        g_control_sock = -1;
        
        ESP_LOGW(TAG, "连接断开，5秒后重连...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief 初始化FRP客户端
 */
esp_err_t xn_frpc_init(const xn_frpc_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_config, config, sizeof(xn_frpc_config_t));
    
    ESP_LOGI(TAG, "FRP客户端初始化");
    ESP_LOGI(TAG, "  服务器: %s:%d", g_config.server_addr, g_config.server_port);
    ESP_LOGI(TAG, "  代理: %s", g_config.proxy_name);
    ESP_LOGI(TAG, "  端口映射: %d -> %d", g_config.remote_port, g_config.local_port);
    
    return ESP_OK;
}

/**
 * @brief 启动FRP客户端
 */
esp_err_t xn_frpc_start(void)
{
    if (g_is_running) {
        ESP_LOGW(TAG, "FRP客户端已在运行");
        return ESP_OK;
    }
    
    g_is_running = true;
    
    xTaskCreate(frpc_main_task, "frpc_main", 8192, NULL, 5, &g_frpc_task);
    
    ESP_LOGI(TAG, "✅ FRP客户端已启动");
    
    return ESP_OK;
}

/**
 * @brief 停止FRP客户端
 */
esp_err_t xn_frpc_stop(void)
{
    g_is_running = false;
    g_is_connected = false;
    
    if (g_control_sock >= 0) {
        close(g_control_sock);
        g_control_sock = -1;
    }
    
    ESP_LOGI(TAG, "FRP客户端已停止");
    
    return ESP_OK;
}

/**
 * @brief 获取连接状态
 */
bool xn_frpc_is_connected(void)
{
    return g_is_connected;
}
