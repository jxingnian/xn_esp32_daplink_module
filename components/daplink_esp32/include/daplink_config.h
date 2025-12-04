/**
 * @file    daplink_config.h
 * @brief   DAPLink ESP32-S3 配置文件
 * 
 * @author  星年
 * @date    2025-12-04
 */

#ifndef DAPLINK_CONFIG_H
#define DAPLINK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 版本信息 ==================== */
#define DAPLINK_VERSION_MAJOR       0
#define DAPLINK_VERSION_MINOR       1
#define DAPLINK_VERSION_PATCH       0

/* ==================== 功能开关 ==================== */
#define ENABLE_SWD                  1       // 启用 SWD 调试
#define ENABLE_JTAG                 0       // 启用 JTAG 调试（暂未实现）
#define ENABLE_CDC                  0       // 启用虚拟串口（暂未实现）
#define ENABLE_MSC                  0       // 启用拖放烧录（暂未实现）
#define ENABLE_SWO                  0       // 启用 SWO 跟踪（暂未实现）

/* ==================== GPIO 引脚配置 ==================== */
// SWD 接口引脚
#define PIN_SWCLK                   GPIO_NUM_1      // SWD 时钟
#define PIN_SWDIO                   GPIO_NUM_2      // SWD 数据

// JTAG 接口引脚
#define PIN_TCK                     GPIO_NUM_7      // JTAG 时钟
#define PIN_TMS                     GPIO_NUM_6      // JTAG 模式选择
#define PIN_TDI                     GPIO_NUM_4      // JTAG 数据输入
#define PIN_TDO                     GPIO_NUM_5      // JTAG 数据输出

// 控制引脚
#define PIN_nRESET                  GPIO_NUM_3      // 目标复位（低电平有效）
#define PIN_SWO                     GPIO_NUM_8      // SWO 串行输出

// 状态指示 LED
#define PIN_LED_CONNECTED           GPIO_NUM_9      // 连接状态 LED
#define PIN_LED_RUNNING             GPIO_NUM_10     // 运行状态 LED（可选）

/* ==================== LED 极性配置 ==================== */
#define LED_ACTIVE_HIGH             1               // LED 高电平点亮
#define LED_ACTIVE_LOW              0               // LED 低电平点亮

#define LED_CONNECTED_POLARITY      LED_ACTIVE_HIGH
#define LED_RUNNING_POLARITY        LED_ACTIVE_HIGH

/* ==================== 时序配置 ==================== */
#define SWD_CLOCK_FREQ_HZ           1000000         // SWD 时钟频率 1MHz
#define JTAG_CLOCK_FREQ_HZ          1000000         // JTAG 时钟频率 1MHz

/* ==================== 缓冲区大小 ==================== */
#define DAP_PACKET_SIZE             64              // DAP 数据包大小
#define DAP_PACKET_COUNT            4               // DAP 数据包数量

/* ==================== 调试配置 ==================== */
#define DAPLINK_DEBUG               1               // 启用调试输出
#define DAPLINK_LOG_LEVEL           3               // 日志级别 (0-4)

#ifdef __cplusplus
}
#endif

#endif /* DAPLINK_CONFIG_H */
