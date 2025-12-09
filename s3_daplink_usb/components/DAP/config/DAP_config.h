/*
 * 版权所有 (c) 2013-2017 ARM Limited. 保留所有权利。
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * 根据 Apache 许可证 2.0 版本（"许可证"）授权；
 * 除非遵守许可证，否则您不得使用此文件。
 * 您可以在以下网址获取许可证副本：
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * 除非适用法律要求或书面同意，否则根据许可证分发的软件
 * 按"原样"分发，不附带任何明示或暗示的担保或条件。
 * 请参阅许可证以了解管理权限和限制的具体语言。
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        2021年6月16日
 * $Revision:    V2.1.0
 *
 * 项目:      CMSIS-DAP 配置
 * 标题:      DAP_config.h CMSIS-DAP 配置文件（模板）
 *
 *---------------------------------------------------------------------------*/

/**
 * @file DAP_config.h
 * @author windowsair
 * @brief GPIO 和 SPI 引脚适配
 * @change: 2021-2-10 支持 GPIO 和 SPI
 *          2021-2-18 尝试支持 SWO
 *          2024-1-28 更新至 CMSIS-DAP v2.1.0
 * @version 0.3
 * @date 2024-1-28
 *
 * @copyright 版权所有 (c) 2021-2024
 *
 */


#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "esp_mac.h"

#include "dap_configuration.h"
#include "timer.h"

#include "cmsis_compiler.h"
#include "gpio_op.h"
#include "spi_switch.h"


// 仅支持 ESP32-S3
#ifndef CONFIG_IDF_TARGET_ESP32S3
  #error "This project only supports ESP32-S3"
#endif



//**************************************************************************************************
/**
\defgroup DAP_Config_Debug_gr CMSIS-DAP 调试单元信息
\ingroup DAP_ConfigIO_gr
@{
提供关于调试单元硬件和配置的定义。

此信息包括：
 - 定义 CMSIS-DAP 调试单元中使用的 Cortex-M 处理器参数。
 - 调试单元标识字符串（厂商、产品、序列号）。
 - 调试单元通信数据包大小。
 - 调试访问端口支持的模式和设置（JTAG/SWD 和 SWO）。
 - 关于连接目标设备的可选信息（用于评估板）。
*/

//#ifdef _RTE_
//#include "RTE_Components.h"
//#include CMSIS_device_header
//#else
//#include "device.h"                             // 调试单元 Cortex-M 处理器头文件
//#endif

/// ESP32-S3 处理器时钟频率（240MHz）
/// 此值用于计算 SWD/JTAG 时钟速度。
#define CPU_CLOCK 240000000



//#define MAX_USER_CLOCK 16000000 ///< 指定最大调试时钟频率（Hz）。

/// I/O 端口写操作所需的处理器周期数。
/// 此值用于计算调试单元中 Cortex-M MCU 通过 I/O 端口写操作生成的 SWD/JTAG 时钟速度。
/// 大多数 Cortex-M 处理器 I/O 端口写操作需要 2 个处理器周期。
/// 如果调试单元使用带有高速外设 I/O 的 Cortex-M0+ 处理器，可能只需要 1 个处理器周期。
#define IO_PORT_WRITE_CYCLES 2U ///< I/O 周期：2=默认，1=Cortex-M0+ 快速 I/O。

/// 指示调试访问端口是否支持串行线调试（SWD）通信模式。
/// 此信息通过 \ref DAP_Info 命令作为 <b>Capabilities</b> 的一部分返回。
#define DAP_SWD 1 ///< SWD 模式：1 = 可用，0 = 不可用。

/// 仅支持 SWD 模式
#define DAP_JTAG 0
#define DAP_JTAG_DEV_CNT 0U
#define DAP_DEFAULT_PORT 1U ///< 默认端口模式：SWD

/// SWD 和 JTAG 模式下调试访问端口的默认通信速度。
/// 用于初始化默认 SWD/JTAG 时钟频率。
/// \ref DAP_SWJ_Clock 命令可用于覆盖此默认设置。
#define DAP_DEFAULT_SWJ_CLOCK 1000000U ///< 默认 SWD/JTAG 时钟频率（Hz）.


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<1MHz

/// 命令和响应数据的最大数据包缓冲区数量。
/// 此配置设置用于优化与调试器的通信性能，取决于 USB 外设。
/// 对于 RAM 或 USB 缓冲区有限的设备，可以减少此设置（有效范围为 1 .. 255）。
#define DAP_PACKET_COUNT 255 ///< 指定缓冲的数据包数量。

/// 指示 SWO 功能（UART SWO 和流式跟踪）是否可用
#define SWO_FUNCTION_ENABLE 0 ///< SWO 功能：1 = 可用，0 = 不可用。


/// 指示 UART 串行线输出（SWO）跟踪是否可用。
/// 此信息通过 \ref DAP_Info 命令作为 <b>Capabilities</b> 的一部分返回。
#define SWO_UART SWO_FUNCTION_ENABLE ///< SWO UART：1 = 可用，0 = 不可用。

/// UART SWO 的 USART 驱动程序实例号。
#define SWO_UART_DRIVER 0 ///< USART 驱动程序实例号（Driver_USART#）。

/// 最大 SWO UART 波特率。
#define SWO_UART_MAX_BAUDRATE (115200U * 40U) ///< SWO UART 最大波特率（Hz）。
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<< 5MHz
//// TODO: 不确定的值

/// 指示曼彻斯特串行线输出（SWO）跟踪是否可用。
/// 此信息通过 \ref DAP_Info 命令作为 <b>Capabilities</b> 的一部分返回。
#define SWO_MANCHESTER 0 ///< SWO 曼彻斯特：1 = 可用，0 = 不可用。
// (windowsair)请勿修改。不支持。


/// SWO 跟踪缓冲区大小。
#define SWO_BUFFER_SIZE 2048U ///< SWO 跟踪缓冲区大小（字节，必须为 2^n）。

/// SWO 流式跟踪。
#define SWO_STREAM SWO_FUNCTION_ENABLE ///< SWO 流式跟踪：1 = 可用，0 = 不可用。

/// 测试域定时器的时钟频率。定时器值通过 \ref TIMESTAMP_GET 返回。
#define TIMESTAMP_CLOCK 5000000U ///< 时间戳时钟频率（Hz）（0 = 不支持时间戳）。
// <<<<<<<<<<<<<<<<<<<<<5MHz

/// 指示 UART 通信端口是否可用。
/// 此信息通过 \ref DAP_Info 命令作为 <b>Capabilities</b> 的一部分返回。
#define DAP_UART                0               ///< DAP UART：1 = 可用，0 = 不可用。

/// UART 通信端口的 USART 驱动程序实例号。
#define DAP_UART_DRIVER         1               ///< USART 驱动程序实例号（Driver_USART#）。

/// UART 接收缓冲区大小。
#define DAP_UART_RX_BUFFER_SIZE 1024U           ///< UART 接收缓冲区大小（字节，必须为 2^n）。

/// UART 发送缓冲区大小。
#define DAP_UART_TX_BUFFER_SIZE 1024U           ///< UART 发送缓冲区大小（字节，必须为 2^n）。

/// 指示是否可通过 USB COM 端口进行 UART 通信。
/// 此信息通过 \ref DAP_Info 命令作为 <b>Capabilities</b> 的一部分返回。
#define DAP_UART_USB_COM_PORT   0               ///< USB COM 端口：1 = 可用，0 = 不可用。

/// 调试单元连接到固定目标设备。
/// 调试单元可能是评估板的一部分，始终连接到固定的已知设备。
/// 在这种情况下，存储设备厂商、设备名称、板卡厂商和板卡名称字符串，
/// 可供调试器或 IDE 用于配置设备参数。
#define TARGET_FIXED            1               ///< 目标：1 = 已知，0 = 未知；

#define TARGET_DEVICE_VENDOR    ""                 ///< 指示芯片厂商的字符串
#define TARGET_DEVICE_NAME      ""                 ///< 指示目标设备的字符串
#define TARGET_BOARD_VENDOR     "windowsair"       ///< 指示板卡厂商的字符串
#define TARGET_BOARD_NAME       "ESP wireless DAP" ///< 指示板卡名称的字符串

#if TARGET_FIXED != 0
#include <string.h>
static const char TargetDeviceVendor [] = TARGET_DEVICE_VENDOR;  // 目标设备厂商字符串
static const char TargetDeviceName   [] = TARGET_DEVICE_NAME;    // 目标设备名称字符串
static const char TargetBoardVendor  [] = TARGET_BOARD_VENDOR;   // 目标板卡厂商字符串
static const char TargetBoardName    [] = TARGET_BOARD_NAME;     // 目标板卡名称字符串
#endif

#define osDelay(n) dap_os_delay(n)  // 操作系统延时宏定义

/**
 * @brief 获取厂商 ID 字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetVendorString(char *str)
{
  // 实际上，Keil 可以通过 USB 获取相应信息，
  // 无需填写此信息。
  // (void)str;
  strcpy(str, "windowsair");
  return (sizeof("windowsair"));
}

/**
 * @brief 获取产品 ID 字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetProductString(char *str)
{
  //(void)str;
  strcpy(str, "CMSIS-DAP v2");
  return (sizeof("CMSIS-DAP v2"));
}

/**
 * @brief 获取序列号字符串。
 *
 * 使用 ESP32-S3 的 MAC 地址作为唯一序列号，格式为 12 位十六进制字符串。
 * 例如: "AABBCCDDEEFF"
 *
 * @param str 指向存储字符串的缓冲区（最大 60 字符）。
 * @return 字符串长度（包括终止 NULL 字符），或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetSerNumString(char *str)
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  sprintf(str, "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return 13U;  /* 12 字符 + 1 NULL */
}

/**
 * @brief 获取目标设备厂商字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetTargetDeviceVendorString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetDeviceVendor);
  len = (uint8_t)(strlen(TargetDeviceVendor) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/**
 * @brief 获取目标设备名称字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetTargetDeviceNameString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetDeviceName);
  len = (uint8_t)(strlen(TargetDeviceName) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/**
 * @brief 获取目标板卡厂商字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetTargetBoardVendorString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetBoardVendor);
  len = (uint8_t)(strlen(TargetBoardVendor) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/**
 * @brief 获取目标板卡名称字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetTargetBoardNameString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetBoardName);
  len = (uint8_t)(strlen(TargetBoardName) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/**
 * @brief 获取产品固件版本字符串。
 *
 * @param str 指向存储字符串的缓冲区指针（最大 60 个字符）。
 * @return 字符串长度（包括终止 NULL 字符）或 0（无字符串）。
 */
__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString (char *str) {
  (void)str;
  return (0U);
}

///@}


// ESP32-S3 引脚定义
// 使用 GPIO8 作为 SWDIO，GPIO9 作为 SWCLK（均串 150Ω 电阻）
#define PIN_SWDIO_MOSI 8   // SWDIO
#define PIN_SWCLK 9        // SWCLK
#define PIN_TDO 10         // JTAG TDO
#define PIN_TDI 11         // JTAG TDI
#define PIN_nTRST 14      // JTAG nTRST (可选)
#define PIN_nRESET 13     // 目标复位引脚


//**************************************************************************************************
/**
\defgroup DAP_Config_PortIO_gr CMSIS-DAP Hardware I/O Pin Access
\ingroup DAP_ConfigIO_gr
@{

Standard I/O Pins of the CMSIS-DAP Hardware Debug Port support standard JTAG mode
and Serial Wire Debug (SWD) mode. In SWD mode only 2 pins are required to implement the debug
interface of a device. The following I/O Pins are provided:

JTAG I/O Pin                 | SWD I/O Pin          | CMSIS-DAP Hardware pin mode
---------------------------- | -------------------- | ---------------------------------------------
TCK: Test Clock              | SWCLK: Clock         | Output Push/Pull
TMS: Test Mode Select        | SWDIO: Data I/O      | Output Push/Pull; Input (for receiving data)
TDI: Test Data Input         |                      | Output Push/Pull
TDO: Test Data Output        |                      | Input
nTRST: Test Reset (optional) |                      | Output Open Drain with pull-up resistor
nRESET: Device Reset         | nRESET: Device Reset | Output Open Drain with pull-up resistor


DAP Hardware I/O Pin Access Functions
-------------------------------------
The various I/O Pins are accessed by functions that implement the Read, Write, Set, or Clear to
these I/O Pins.

For the SWDIO I/O Pin there are additional functions that are called in SWD I/O mode only.
This functions are provided to achieve faster I/O that is possible with some advanced GPIO
peripherals that can independently write/read a single I/O pin without affecting any other pins
of the same I/O port. The following SWDIO I/O Pin functions are provided:
 - \ref PIN_SWDIO_OUT_ENABLE to enable the output mode from the DAP hardware.
 - \ref PIN_SWDIO_OUT_DISABLE to enable the input mode to the DAP hardware.
 - \ref PIN_SWDIO_IN to read from the SWDIO I/O pin with utmost possible speed.
 - \ref PIN_SWDIO_OUT to write to the SWDIO I/O pin with utmost possible speed.
*/

/**
 * @brief Setup SWD I/O pins: SWCLK, SWDIO, and nRESET.
 * Configures the DAP Hardware I/O pins for Serial Wire Debug (SWD) mode:
 * - SWCLK, SWDIO, nRESET to output mode and set to default high level.
 * - TDI, nTRST to HighZ mode (pins are unused in SWD mode).
 *
 */
__STATIC_INLINE void PORT_SWD_SETUP(void)
{
  // We will switch to the specific mode when setting the transfer rate.

  // Now we need to set it to ordinary GPIO mode. In most implementations,
  // the DAP will then read the status of the PIN via the `SWJ_PIN` command.
  DAP_SPI_Deinit();
}

/**
 * @brief Disable JTAG/SWD I/O Pins.
 * Disables the DAP Hardware I/O pins which configures:
 * - TCK/SWCLK, TMS/SWDIO, TDI, TDO, nTRST, nRESET to High-Z mode.
 *
 */
__STATIC_INLINE void PORT_OFF(void)
{
  // DAP 断开连接时调用
  gpio_ll_output_enable(&GPIO, PIN_nRESET);
  gpio_ll_od_enable(&GPIO, PIN_nRESET);
  GPIO_PULL_UP_ONLY_SET(PIN_nRESET);
  gpio_ll_set_level(&GPIO, PIN_nRESET, 1);
}

// SWCLK/TCK I/O pin -------------------------------------

/**
 * @brief SWCLK/TCK I/O pin: Get Input.
 *
 * @return Current status of the SWCLK/TCK DAP hardware I/O pin.
 */
__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN(void)
{
  ////TODO: can we set to 0?
  return 0;
}

/**
 * @brief SWCLK/TCK I/O pin: Set Output to High.
 *
 *  Set the SWCLK/TCK DAP hardware I/O pin to high level.
 */
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_SET(void)
{
#ifdef SWCLK_SET
  SWCLK_SET();
#else
  GPIO_SET_LEVEL_HIGH(PIN_SWCLK);
#endif
}

/**
 * @brief SWCLK/TCK I/O pin: Set Output to Low.
 *
 *  Set the SWCLK/TCK DAP hardware I/O pin to low level.
 */
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_CLR(void)
{
#ifdef SWCLK_CLR
  SWCLK_CLR();
#else
  GPIO_SET_LEVEL_LOW(PIN_SWCLK);
#endif
}

// SWDIO/TMS Pin I/O --------------------------------------

/**
 * @brief SWDIO/TMS I/O pin: Get Input.
 *
 * @return Current status of the SWDIO/TMS DAP hardware I/O pin.
 */
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN(void)
{
  // Note that we only use mosi in GPIO mode
#ifdef SWDIO_GET_IN
  return SWDIO_GET_IN();
#else
  return GPIO_GET_LEVEL(PIN_SWDIO_MOSI);
#endif
}

/**
 * @brief SWDIO/TMS I/O pin: Set Output to High.
 *
 * Set the SWDIO/TMS DAP hardware I/O pin to high level.
 */
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_SET(void)
{
#ifdef SWDIO_SET
  SWDIO_SET();
#else
  GPIO_SET_LEVEL_HIGH(PIN_SWDIO_MOSI);
#endif
}

/**
 * @brief SWDIO/TMS I/O pin: Set Output to Low.
 *
 * Set the SWDIO/TMS DAP hardware I/O pin to low level.
 */
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_CLR(void)
{
#ifdef SWDIO_CLR
  SWDIO_CLR();
#else
  GPIO_SET_LEVEL_LOW(PIN_SWDIO_MOSI);
#endif
}

/**
 * @brief SWDIO I/O 引脚：获取输入（仅用于 SWD 模式）。
 *
 * @return SWDIO DAP 硬件 I/O 引脚的当前状态。
 */
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN(void)
{
  return PIN_SWDIO_TMS_IN();
}

/**
 * @brief SWDIO I/O 引脚：设置输出（仅用于 SWD 模式）。
 *
 * @param bit SWDIO DAP 硬件 I/O 引脚的输出值。
 *
 */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT(uint32_t bit)
{
  /**
    * 重要：仅使用参数的一位（bit0）！
    * 有时 SW_DP.c 中的 "SWD_TransferFunction" 函数会
    * 传入 "2" 而不是 "0" 作为参数。Zach Lee
    */
  if ((bit & 1U) == 1)
  {
    PIN_SWDIO_TMS_SET();
  }
  else
  {
    PIN_SWDIO_TMS_CLR();
  }
}

/**
 * @brief SWDIO I/O 引脚：切换到输出模式（仅用于 SWD 模式）。
 */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_ENABLE(void)
{
  gpio_ll_output_enable(&GPIO, PIN_SWDIO_MOSI);
}

/**
 * @brief SWDIO I/O 引脚：切换到输入模式（仅用于 SWD 模式）。
 */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_DISABLE(void)
{
  gpio_ll_output_disable(&GPIO, PIN_SWDIO_MOSI);
  gpio_ll_input_enable(&GPIO, PIN_SWDIO_MOSI);
}

// JTAG 引脚空实现（CMSIS-DAP 协议需要，即使只用 SWD）
__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void) { return 0; }
__STATIC_FORCEINLINE void PIN_TDI_OUT(uint32_t bit) { (void)bit; }
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void) { return 0; }
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void) { return 0; }
__STATIC_FORCEINLINE void PIN_nTRST_OUT(uint32_t bit) { (void)bit; }

// nRESET 引脚 I/O------------------------------------------

/**
 * @brief nRESET I/O 引脚：获取输入。
 *
 * @return nRESET DAP 硬件 I/O 引脚的当前状态。
 */
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void)
{
  return GPIO_GET_LEVEL(PIN_nRESET);
}

/**
 * @brief nRESET I/O 引脚：设置输出。
 *
 * @param bit 目标设备硬件复位引脚状态：
 *            - 0：触发设备硬件复位。
 *            - 1：释放设备硬件复位。
 */
__STATIC_FORCEINLINE void PIN_nRESET_OUT(uint32_t bit)
{
  if ((bit & 1U) == 1)
  {
    // 释放复位
    GPIO_SET_LEVEL_HIGH(PIN_nRESET);
    gpio_ll_output_disable(&GPIO, PIN_nRESET);
  }
  else
  {
    // 触发复位
    gpio_ll_output_enable(&GPIO, PIN_nRESET);
    GPIO_SET_LEVEL_LOW(PIN_nRESET);
  }
}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_LEDs_gr CMSIS-DAP 硬件状态 LED
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件可以提供 LED 来指示 CMSIS-DAP 调试单元的状态。

建议提供以下 LED 用于状态指示：
 - 连接 LED：当 DAP 硬件连接到调试器时激活。
 - 运行 LED：当调试器将目标设备置于运行状态时激活。
*/

/**
 * @brief 调试单元：设置连接 LED 的状态。
 *
 * 此函数用于控制指示调试器连接状态的 LED。
 * 当调试器（如 IDE）与 CMSIS-DAP 调试单元建立连接时，
 * 主机会发送 DAP_HostStatus 命令来更新此 LED 状态。
 *
 * @param bit 连接 LED 的状态。
 *        - 1: 连接 LED 点亮：调试器已连接到 CMSIS-DAP 调试单元。
 *        - 0: 连接 LED 熄灭：调试器未连接到 CMSIS-DAP 调试单元。
 *
 * @note 当前实现为空，如需启用 LED 指示功能，请根据硬件配置实现此函数。
 * @see DAP_HostStatus() 调用此函数来更新连接状态
 */
__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit)
{
  (void)(bit);
  // 示例实现：
  // if (bit) {
  //   GPIO_SET_LEVEL_HIGH(PIN_LED_CONNECTED);
  // } else {
  //   GPIO_SET_LEVEL_LOW(PIN_LED_CONNECTED);
  // }
}

/**
 * @brief 调试单元：设置目标运行 LED 的状态。
 *
 * 此函数用于控制指示目标设备运行状态的 LED。
 * 当调试器控制目标设备开始或停止程序执行时，
 * 主机会发送 DAP_HostStatus 命令来更新此 LED 状态。
 *
 * @param bit 目标运行 LED 的状态。
 *        - 1: 目标运行 LED 点亮：目标设备程序执行已开始。
 *        - 0: 目标运行 LED 熄灭：目标设备程序执行已停止。
 *
 * @note 当前实现为空，如需启用 LED 指示功能，请根据硬件配置实现此函数。
 * @see DAP_HostStatus() 调用此函数来更新运行状态
 */
__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit)
{
  (void)(bit);
  // 示例实现：
  // if (bit) {
  //   // 设置引脚为高电平，点亮 LED
  //   GPIO.out_w1ts |= (0x1 << PIN_LED_RUNNING);
  // } else {
  //   // 设置引脚为低电平，熄灭 LED
  //   GPIO.out_w1tc |= (0x1 << PIN_LED_RUNNING);
  // }
}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_Timestamp_gr CMSIS-DAP 时间戳
\ingroup DAP_ConfigIO_gr
@{
测试域定时器访问函数。

调试单元中测试域定时器的值由函数 \ref TIMESTAMP_GET 返回。
默认使用 DWT 定时器。此定时器的频率由 \ref TIMESTAMP_CLOCK 配置。

*/

/**
 * @brief 获取测试域定时器的时间戳。
 *
 * @return 当前时间戳值。
 */
__STATIC_INLINE uint32_t TIMESTAMP_GET(void)
{
  return get_timer_count();
}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_Initialization_gr CMSIS-DAP 初始化
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件 I/O 和 LED 引脚通过函数 \ref DAP_SETUP 进行初始化。
*/

/** 调试单元 I/O 引脚和 LED 的设置（在调试单元初始化时调用）。
此函数执行 CMSIS-DAP 硬件 I/O 引脚和状态 LED 的初始化。
详细操作包括启用并设置硬件 I/O 和 LED 引脚：
 - 启用 I/O 时钟系统。
 - 所有 I/O 引脚：启用输入缓冲区，输出引脚设置为高阻态模式。
 - 对于 nTRST、nRESET，启用弱上拉（如果可用）。
 - 启用 LED 输出引脚并关闭 LED。
*/
__STATIC_INLINE void DAP_SETUP(void)
{
  // 初始化 SWD 引脚
  GPIO_FUNCTION_SET(PIN_SWCLK);
  GPIO_FUNCTION_SET(PIN_SWDIO_MOSI);
  GPIO_FUNCTION_SET(PIN_nRESET);

  // 设置驱动能力 (5mA)
  gpio_ll_set_drive_capability(&GPIO, PIN_SWCLK, GPIO_DRIVE_CAP_0);
  gpio_ll_set_drive_capability(&GPIO, PIN_SWDIO_MOSI, GPIO_DRIVE_CAP_0);

  PORT_OFF();
}

extern void dap_os_delay(int ms);
/** 使用自定义特定 I/O 引脚或命令序列复位目标设备。
此函数允许可选地实现设备特定的复位序列。
当执行 \ref DAP_ResetTarget 命令时调用此函数，
例如当设备需要时间关键的解锁序列来启用调试端口时需要此函数。
\return 0 = 未实现设备特定的复位序列。\n
        1 = 已实现设备特定的复位序列。
*/
__STATIC_INLINE uint8_t RESET_TARGET(void)
{

  PIN_nRESET_OUT(0);
  dap_os_delay(2);
  PIN_nRESET_OUT(1);
  dap_os_delay(2);
  return (1U); // 成功
}

///@}

#endif /* __DAP_CONFIG_H__ */
