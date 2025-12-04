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

// 注意：ESP32-S3 使用不同的寄存器地址，需要包含 soc/gpio_reg.h
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/soc.h"

/* ==================== SWD 引脚操作宏 ==================== */

/// SWCLK 引脚设置为高电平（直接寄存器操作，2-3 CPU周期）
static inline void PIN_SWCLK_SET_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << PIN_SWCLK));
}
#define PIN_SWCLK_SET()         PIN_SWCLK_SET_INLINE()

/// SWCLK 引脚设置为低电平
static inline void PIN_SWCLK_CLR_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << PIN_SWCLK));
}
#define PIN_SWCLK_CLR()         PIN_SWCLK_CLR_INLINE()

/// SWDIO 引脚设置为输出模式（初始化时调用，不需要高速）
#define PIN_SWDIO_OUT_ENABLE()  gpio_set_direction(PIN_SWDIO, GPIO_MODE_OUTPUT)

/// SWDIO 引脚设置为输入模式
#define PIN_SWDIO_OUT_DISABLE() gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT)

/// SWDIO 引脚设置为高电平（直接寄存器操作）
static inline void PIN_SWDIO_SET_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << PIN_SWDIO));
}
#define PIN_SWDIO_SET()         PIN_SWDIO_SET_INLINE()

/// SWDIO 引脚设置为低电平
static inline void PIN_SWDIO_CLR_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << PIN_SWDIO));
}
#define PIN_SWDIO_CLR()         PIN_SWDIO_CLR_INLINE()

/// 读取 SWDIO 引脚电平（直接寄存器读取）
static inline uint32_t PIN_SWDIO_IN_INLINE(void) {
    return (REG_READ(GPIO_IN_REG) >> PIN_SWDIO) & 1UL;
}
#define PIN_SWDIO_IN()          PIN_SWDIO_IN_INLINE()

/* ==================== JTAG 引脚操作宏 ==================== */

#if DAP_JTAG

/// TCK 引脚设置为高电平
static inline void PIN_TCK_SET_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << PIN_TCK));
}
#define PIN_TCK_SET()           PIN_TCK_SET_INLINE()

/// TCK 引脚设置为低电平
static inline void PIN_TCK_CLR_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << PIN_TCK));
}
#define PIN_TCK_CLR()           PIN_TCK_CLR_INLINE()

/// TMS 引脚设置为高电平
static inline void PIN_TMS_SET_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << PIN_TMS));
}
#define PIN_TMS_SET()           PIN_TMS_SET_INLINE()

/// TMS 引脚设置为低电平
static inline void PIN_TMS_CLR_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << PIN_TMS));
}
#define PIN_TMS_CLR()           PIN_TMS_CLR_INLINE()

/// TDI 引脚设置为高电平
static inline void PIN_TDI_SET_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << PIN_TDI));
}
#define PIN_TDI_SET()           PIN_TDI_SET_INLINE()

/// TDI 引脚设置为低电平
static inline void PIN_TDI_CLR_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << PIN_TDI));
}
#define PIN_TDI_CLR()           PIN_TDI_CLR_INLINE()

/// 读取 TDO 引脚电平
static inline uint32_t PIN_TDO_IN_INLINE(void) {
    return (REG_READ(GPIO_IN_REG) >> PIN_TDO) & 1UL;
}
#define PIN_TDO_IN()            PIN_TDO_IN_INLINE()

#endif

/* ==================== 复位引脚操作宏 ==================== */

/// nRESET 引脚设置为高电平（释放复位）
static inline void PIN_nRESET_SET_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << PIN_nRESET));
}
#define PIN_nRESET_SET()        PIN_nRESET_SET_INLINE()

/// nRESET 引脚设置为低电平（进入复位）
static inline void PIN_nRESET_CLR_INLINE(void) {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << PIN_nRESET));
}
#define PIN_nRESET_CLR()        PIN_nRESET_CLR_INLINE()

/// 读取 nRESET 引脚电平
static inline uint32_t PIN_nRESET_IN_INLINE(void) {
    return (REG_READ(GPIO_IN_REG) >> PIN_nRESET) & 1UL;
}
#define PIN_nRESET_IN()         PIN_nRESET_IN_INLINE()

/* ==================== LED 操作宏 ==================== */

// LED 不需要高速操作，使用 HAL 函数即可
#if LED_CONNECTED_POLARITY == LED_ACTIVE_HIGH
    #define LED_CONNECTED_ON()  gpio_set_level(PIN_LED_CONNECTED, 1)
    #define LED_CONNECTED_OFF() gpio_set_level(PIN_LED_CONNECTED, 0)
#else
    #define LED_CONNECTED_ON()  gpio_set_level(PIN_LED_CONNECTED, 0)
    #define LED_CONNECTED_OFF() gpio_set_level(PIN_LED_CONNECTED, 1)
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
