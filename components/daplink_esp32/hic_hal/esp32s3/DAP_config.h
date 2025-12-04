/**
 * @file    DAP_config.h
 * @brief   CMSIS-DAP 配置文件 (ESP32-S3)
 * 
 * @author  星年
 * @date    2025-12-04
 */

#ifndef DAP_CONFIG_H
#define DAP_CONFIG_H

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "daplink_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== DAP 基本配置 ==================== */

/// DAP 硬件厂商字符串
#define DAP_VENDOR              "XingNian"

/// DAP 产品名称字符串
#define DAP_PRODUCT             "ESP32-S3 CMSIS-DAP"

/// DAP 序列号字符串
#define DAP_SER_NUM             "0001"

/// DAP 固件版本字符串
#define DAP_FW_VER              "0.1.0"

/// DAP 设备厂商 ID
#define DAP_VENDOR_ID           0x0D28

/// DAP 设备产品 ID
#define DAP_PRODUCT_ID          0x0204

/* ==================== DAP 功能配置 ==================== */

/// DAP 最大数据包大小
#define DAP_PACKET_SIZE         64

/// DAP 数据包缓冲区数量
#define DAP_PACKET_COUNT        4

/// 支持 SWD 协议
#define DAP_SWD                 1

/// 支持 JTAG 协议
#define DAP_JTAG                0

/// 支持 SWO UART 模式
#define SWO_UART                0

/// 支持 SWO Manchester 模式
#define SWO_MANCHESTER          0

/// 支持原子命令
#define DAP_ATOMIC_COMMANDS     0

/* ==================== GPIO 宏定义 ==================== */

// GPIO 寄存器直接访问
#define GPIO_OUT_SET_REG        GPIO.out_w1ts
#define GPIO_OUT_CLR_REG        GPIO.out_w1tc
#define GPIO_IN_REG             GPIO.in

/* ==================== SWD 引脚操作宏 ==================== */

/// SWCLK 引脚设置为高电平
#define PIN_SWCLK_SET()         GPIO_OUT_SET_REG = (1ULL << PIN_SWCLK)

/// SWCLK 引脚设置为低电平
#define PIN_SWCLK_CLR()         GPIO_OUT_CLR_REG = (1ULL << PIN_SWCLK)

/// SWDIO 引脚设置为输出模式
#define PIN_SWDIO_OUT_ENABLE()  do { \
    gpio_set_direction(PIN_SWDIO, GPIO_MODE_OUTPUT); \
} while(0)

/// SWDIO 引脚设置为输入模式
#define PIN_SWDIO_OUT_DISABLE() do { \
    gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT); \
} while(0)

/// SWDIO 引脚设置为高电平
#define PIN_SWDIO_SET()         GPIO_OUT_SET_REG = (1ULL << PIN_SWDIO)

/// SWDIO 引脚设置为低电平
#define PIN_SWDIO_CLR()         GPIO_OUT_CLR_REG = (1ULL << PIN_SWDIO)

/// 读取 SWDIO 引脚电平
#define PIN_SWDIO_IN()          ((GPIO_IN_REG >> PIN_SWDIO) & 1)

/* ==================== JTAG 引脚操作宏 ==================== */

#if DAP_JTAG

/// TCK 引脚设置为高电平
#define PIN_TCK_SET()           GPIO_OUT_SET_REG = (1ULL << PIN_TCK)

/// TCK 引脚设置为低电平
#define PIN_TCK_CLR()           GPIO_OUT_CLR_REG = (1ULL << PIN_TCK)

/// TMS 引脚设置为高电平
#define PIN_TMS_SET()           GPIO_OUT_SET_REG = (1ULL << PIN_TMS)

/// TMS 引脚设置为低电平
#define PIN_TMS_CLR()           GPIO_OUT_CLR_REG = (1ULL << PIN_TMS)

/// TDI 引脚设置为高电平
#define PIN_TDI_SET()           GPIO_OUT_SET_REG = (1ULL << PIN_TDI)

/// TDI 引脚设置为低电平
#define PIN_TDI_CLR()           GPIO_OUT_CLR_REG = (1ULL << PIN_TDI)

/// 读取 TDO 引脚电平
#define PIN_TDO_IN()            ((GPIO_IN_REG >> PIN_TDO) & 1)

#endif

/* ==================== 复位引脚操作宏 ==================== */

/// nRESET 引脚设置为高电平（释放复位）
#define PIN_nRESET_SET()        GPIO_OUT_SET_REG = (1ULL << PIN_nRESET)

/// nRESET 引脚设置为低电平（进入复位）
#define PIN_nRESET_CLR()        GPIO_OUT_CLR_REG = (1ULL << PIN_nRESET)

/// 读取 nRESET 引脚电平
#define PIN_nRESET_IN()         ((GPIO_IN_REG >> PIN_nRESET) & 1)

/* ==================== LED 操作宏 ==================== */

#if LED_CONNECTED_POLARITY == LED_ACTIVE_HIGH
    #define LED_CONNECTED_ON()  GPIO_OUT_SET_REG = (1ULL << PIN_LED_CONNECTED)
    #define LED_CONNECTED_OFF() GPIO_OUT_CLR_REG = (1ULL << PIN_LED_CONNECTED)
#else
    #define LED_CONNECTED_ON()  GPIO_OUT_CLR_REG = (1ULL << PIN_LED_CONNECTED)
    #define LED_CONNECTED_OFF() GPIO_OUT_SET_REG = (1ULL << PIN_LED_CONNECTED)
#endif

/* ==================== 时序延时宏 ==================== */

/// 延时 N 个 CPU 周期（用于精确时序）
#define CPU_DELAY_CYCLES(n)     do { \
    for (volatile uint32_t i = 0; i < (n); i++) { \
        __asm__ __volatile__("nop"); \
    } \
} while(0)

/// 微秒延时
#define DELAY_US(us)            esp_rom_delay_us(us)

/* ==================== 时钟配置 ==================== */

/// DAP 默认时钟频率 (Hz)
#define DAP_DEFAULT_SWJ_CLOCK   1000000

/// DAP 最大时钟频率 (Hz)
#define DAP_MAX_SWJ_CLOCK       10000000

/// DAP 最小时钟频率 (Hz)
#define DAP_MIN_SWJ_CLOCK       100000

/* ==================== 函数声明 ==================== */

/**
 * @brief DAP 端口初始化
 */
void PORT_DAP_SETUP(void);

/**
 * @brief 设置 DAP 时钟频率
 * @param clock 时钟频率 (Hz)
 * @return 实际设置的时钟频率
 */
uint32_t PORT_SWJ_CLOCK_SET(uint32_t clock);

/**
 * @brief 连接到目标设备
 * @param port 端口类型 (1: SWD, 2: JTAG)
 */
void PORT_SWJ_CONNECT(uint32_t port);

/**
 * @brief 断开与目标设备的连接
 */
void PORT_SWJ_DISCONNECT(void);

#ifdef __cplusplus
}
#endif

#endif /* DAP_CONFIG_H */
