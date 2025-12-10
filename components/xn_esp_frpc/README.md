<!--
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-10 09:54:21
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-10 10:28:02
 * @FilePath: \todo-xn_esp32_daplink_module\components\xn_esp_frpc\README.md
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
-->
# xn_esp_frpc - ESP32 FRP客户端组件

简化版FRP客户端，专用于ESP32-S3的DAP端口映射。

## 功能

- 连接FRP服务器
- TCP端口映射
- 自动重连
- 心跳保活

## 使用方法

```c
#include "xn_esp_frpc.h"

// 配置FRP客户端
xn_frpc_config_t frpc_config = {
    .server_addr = "frp.example.com",
    .server_port = 7000,
    .auth_token = "your_token",
    .proxy_name = "esp32_dap",
    .local_port = 5555,      // 本地DAP服务端口
    .remote_port = 5555,     // 远程映射端口
    .heartbeat_interval = 30,
};

// 初始化
xn_frpc_init(&frpc_config);

// 启动
xn_frpc_start();
```

## 配置说明

- `server_addr`: FRP服务器地址
- `server_port`: FRP服务器端口（默认7000）
- `auth_token`: 认证token（与服务器配置一致）
- `proxy_name`: 代理名称（唯一标识）
- `local_port`: 本地服务端口（DAP TCP服务器端口）
- `remote_port`: 远程端口（外网访问端口）

## 依赖

- ESP-IDF >= 4.4.0
- lwip

## 注意事项

1. 需要先启动本地DAP TCP服务器（端口5555）
2. FRP服务器需要正确配置
3. 确保4G网络已连接
