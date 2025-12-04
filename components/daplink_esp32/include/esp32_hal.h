/**
 * @file    esp32_hal.h
 * @brief   ESP32-S3 硬件抽象层接口定义
 * 
 * @author  星年
 * @date    2025-12-04
 */

#ifndef ESP32_HAL_H
#define ESP32_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== GPIO 接口 ==================== */

/**
 * @brief GPIO 初始化
 * @return 0: 成功, -1: 失败
 */
int gpio_hal_init(void);

/**
 * @brief 设置 LED 状态
 * @param led_id LED ID (0: 连接LED, 1: 运行LED)
 * @param state LED 状态 (true: 点亮, false: 熄灭)
 */
void gpio_hal_set_led(uint8_t led_id, bool state);

/**
 * @brief LED 闪烁
 * @param led_id LED ID
 * @param count 闪烁次数
 * @param delay_ms 闪烁间隔(毫秒)
 */
void gpio_hal_led_blink(uint8_t led_id, uint8_t count, uint32_t delay_ms);

/**
 * @brief 设置目标复位引脚状态
 * @param state true: 复位, false: 释放
 */
void gpio_hal_set_reset(bool state);

/**
 * @brief 读取目标复位引脚状态
 * @return true: 复位中, false: 正常
 */
bool gpio_hal_get_reset(void);

/* ==================== UART 接口 ==================== */

/**
 * @brief UART 初始化
 * @param uart_num UART 端口号
 * @param baud_rate 波特率
 * @return 0: 成功, -1: 失败
 */
int uart_hal_init(uint8_t uart_num, uint32_t baud_rate);

/**
 * @brief UART 发送数据
 * @param uart_num UART 端口号
 * @param data 数据指针
 * @param len 数据长度
 * @return 实际发送的字节数
 */
int uart_hal_write(uint8_t uart_num, const uint8_t *data, uint32_t len);

/**
 * @brief UART 接收数据
 * @param uart_num UART 端口号
 * @param data 数据缓冲区
 * @param len 缓冲区大小
 * @param timeout_ms 超时时间(毫秒)
 * @return 实际接收的字节数
 */
int uart_hal_read(uint8_t uart_num, uint8_t *data, uint32_t len, uint32_t timeout_ms);

/* ==================== USB 缓冲区接口 ==================== */

/**
 * @brief USB 缓冲区初始化
 * @return 0: 成功, -1: 失败
 */
int usb_buf_init(void);

/**
 * @brief 写入 USB 发送缓冲区
 * @param data 数据指针
 * @param len 数据长度
 * @return 实际写入的字节数
 */
int usb_buf_write(const uint8_t *data, uint32_t len);

/**
 * @brief 从 USB 接收缓冲区读取
 * @param data 数据缓冲区
 * @param len 缓冲区大小
 * @return 实际读取的字节数
 */
int usb_buf_read(uint8_t *data, uint32_t len);

/* ==================== 系统接口 ==================== */

/**
 * @brief 获取系统时间戳(微秒)
 * @return 时间戳
 */
uint64_t system_get_time_us(void);

/**
 * @brief 延时(微秒)
 * @param us 延时时间
 */
void system_delay_us(uint32_t us);

/**
 * @brief 延时(毫秒)
 * @param ms 延时时间
 */
void system_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_HAL_H */
