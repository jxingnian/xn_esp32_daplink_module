/**
 * @file    DAP_config.h
 * @brief   ESP32-S3 CMSIS-DAP 配置文件
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2009-2021, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

//**************************************************************************************************
/**
\defgroup DAP_Config_Debug_gr CMSIS-DAP 调试单元信息
\ingroup DAP_ConfigIO_gr
@{
提供有关调试单元的硬件和配置的定义。

此信息包括:
 - CMSIS-DAP 调试单元中使用的 Cortex-M 处理器参数的定义。
 - 调试单元标识字符串(供应商、产品、序列号)。
 - 调试单元通信数据包大小。
 - 调试访问端口支持的模式和设置(JTAG/SWD 和 SWO)。
 - 关于连接的目标设备的可选信息(用于评估板)。
*/

#include "esp32s3/rom/gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(__GNUC__) && !defined(__STATIC_FORCEINLINE)
#define __STATIC_FORCEINLINE static inline __attribute__((always_inline))
#endif

#if defined(__GNUC__) && !defined(__STATIC_INLINE)
#define __STATIC_INLINE static inline
#endif

/// 调试单元中使用的 Cortex-M MCU 的处理器时钟。
/// 此值用于计算 SWD/JTAG 时钟速度。
#define CPU_CLOCK               240000000U        ///< 指定 CPU 时钟(Hz)

/// I/O 端口写操作的处理器周期数。
/// 此值用于计算调试单元中 Cortex-M MCU 通过 I/O 端口写操作生成的 SWD/JTAG 时钟速度。
/// 大多数 Cortex-M 处理器需要 2 个处理器周期进行 I/O 端口写操作。
/// 如果调试单元使用具有高速外设 I/O 的 Cortex-M0+ 处理器,可能只需要 1 个处理器周期。
#define IO_PORT_WRITE_CYCLES    1U              ///< I/O 周期: 2=默认, 1=Cortex-M0+ 快速 I/0

/// 指示调试访问端口是否支持串行线调试(SWD)通信模式。
/// 此信息作为<b>功能</b>的一部分由命令 \ref DAP_Info 返回。
#ifndef DAP_SWD
#define DAP_SWD                 1               ///< SWD 模式: 1 = 可用, 0 = 不可用
#endif

/// 指示调试端口是否支持 JTAG 通信模式。
/// 此信息作为<b>功能</b>的一部分由命令 \ref DAP_Info 返回。
#ifndef DAP_JTAG
#define DAP_JTAG                0               ///< JTAG 模式: 1 = 可用, 0 = 不可用
#endif

/// 配置连接到调试访问端口的扫描链上的最大 JTAG 设备数。
/// 此设置影响调试单元的 RAM 要求。有效范围是 1 .. 255。
#define DAP_JTAG_DEV_CNT        8               ///< 扫描链上的最大 JTAG 设备数

/// 调试访问端口的默认通信模式。
/// 当选择端口默认模式时用于命令 \ref DAP_Connect。
#if (DAP_SWD == 1)
#define DAP_DEFAULT_PORT        1               ///< 默认 JTAG/SWJ 端口模式: 1 = SWD, 2 = JTAG
#elif (DAP_JTAG == 1)
#define DAP_DEFAULT_PORT        2               ///< 默认 JTAG/SWJ 端口模式: 1 = SWD, 2 = JTAG
#else
#error 必须启用 DAP_SWD 和/或 DAP_JTAG
#endif

/// SWD 和 JTAG 模式下调试访问端口的默认通信速度。
/// 用于初始化默认 SWD/JTAG 时钟频率。
/// 命令 \ref DAP_SWJ_Clock 可用于覆盖此默认设置。
#define DAP_DEFAULT_SWJ_CLOCK   4000000U         ///< 默认 SWD/JTAG 时钟频率(Hz)

/// 命令和响应数据的最大数据包大小。
/// 此配置设置用于优化与调试器的通信性能,取决于 USB 外设。
/// 典型值为:全速 USB HID 或 WinUSB 为 64,高速 USB HID 为 1024,高速 USB WinUSB 为 512。
#define DAP_PACKET_SIZE         64              ///< 指定数据包大小(字节)

/// 命令和响应数据的最大数据包缓冲区数。
/// 此配置设置用于优化与调试器的通信性能,取决于 USB 外设。
/// 对于 RAM 或 USB 缓冲区有限的设备,可以减小设置(有效范围为 1 .. 255)。
/// 对于高速 USB,将设置更改为 4。
#define DAP_PACKET_COUNT        1              ///< 缓冲区: 64 = 全速, 4 = 高速

/// 指示是否支持 UART 串行线输出(SWO)跟踪。
/// 此信息作为<b>功能</b>的一部分由命令 \ref DAP_Info 返回。
#define SWO_UART                0               ///< SWO UART: 1 = 可用, 0 = 不可用

/// UART SWO 的 USART 驱动程序实例号。
#define SWO_UART_DRIVER         0               ///< USART 驱动程序实例号(Driver_USART#)

/// 最大 SWO UART 波特率
#define SWO_UART_MAX_BAUDRATE   10000000U       ///< SWO UART 最大波特率(Hz)

/// 指示是否支持曼彻斯特编码串行线输出(SWO)跟踪。
/// 此信息作为<b>功能</b>的一部分由命令 \ref DAP_Info 返回。
#define SWO_MANCHESTER          0               ///< SWO 曼彻斯特: 1 = 可用, 0 = 不可用

/// SWO 跟踪缓冲区大小。
#define SWO_BUFFER_SIZE         8192U           ///< SWO 跟踪缓冲区大小(字节,必须为 2^n)

/// SWO 流式跟踪。
#define SWO_STREAM              0               ///< SWO 流式跟踪: 1 = 可用, 0 = 不可用

/// 测试域定时器的时钟频率。定时器值通过 \ref TIMESTAMP_GET 返回。
#define TIMESTAMP_CLOCK         240000000U      ///< 时间戳时钟(Hz)(0 = 不支持时间戳)
// DAPLink: 禁用,因为我们使用 DWT 进行时间戳,而 M0 没有 DWT。

/// 指示是否支持 UART 通信端口。
/// 此信息作为<b>功能</b>的一部分由命令 \ref DAP_Info 返回。
#define DAP_UART                0               ///< DAP UART: 1 = 可用, 0 = 不可用

/// UART 通信端口的 USART 驱动程序实例号。
#define DAP_UART_DRIVER         1               ///< USART 驱动程序实例号(Driver_USART#)

/// UART 接收缓冲区大小。
#define DAP_UART_RX_BUFFER_SIZE 1024U           ///< UART 接收缓冲区大小(字节,必须为 2^n)

/// UART 发送缓冲区大小。
#define DAP_UART_TX_BUFFER_SIZE 1024U           ///< UART 发送缓冲区大小(字节,必须为 2^n)

/// 指示是否支持通过 USB COM 端口的 UART 通信。
/// 此信息作为<b>功能</b>的一部分由命令 \ref DAP_Info 返回。
#define DAP_UART_USB_COM_PORT   1               ///< USB COM 端口: 1 = 可用, 0 = 不可用

/// 调试单元是否连接到固定目标设备。
/// 调试单元可能是评估板的一部分,始终连接到已知设备。
/// 在这种情况下,存储设备供应商、设备名称、板卡供应商和板卡名称字符串,
/// 调试器或 IDE 可以使用这些字符串来配置设备参数。
#define TARGET_FIXED            0               ///< 目标: 1 = 已知, 0 = 未知

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_PortIO_gr CMSIS-DAP 硬件 I/O 引脚访问
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件调试端口的标准 I/O 引脚支持标准 JTAG 模式和串行线调试(SWD)模式。
在 SWD 模式下,只需要 2 个引脚即可实现设备的调试接口。提供以下 I/O 引脚:

JTAG I/O 引脚              | SWD I/O 引脚          | CMSIS-DAP 硬件引脚模式
---------------------------- | -------------------- | ---------------------------------------------
TCK: 测试时钟               | SWCLK: 时钟          | 输出推挽
TMS: 测试模式选择           | SWDIO: 数据 I/O      | 输出推挽;输入(用于接收数据)
TDI: 测试数据输入          |                      | 输出推挽
TDO: 测试数据输出          |                      | 输入
nTRST: 测试复位(可选)      |                      | 带上拉电阻的开漏输出
nRESET: 设备复位           | nRESET: 设备复位     | 带上拉电阻的开漏输出


DAP 硬件 I/O 引脚访问函数
-------------------------------------
通过实现对这些 I/O 引脚的读取、写入、设置或清除的函数来访问各种 I/O 引脚。

对于 SWDIO I/O 引脚,还提供了仅在 SWD I/O 模式下调用的其他函数。
提供这些函数是为了实现更快的 I/O,这在一些高级 GPIO 外设中是可能的,
这些外设可以独立写入/读取单个 I/O 引脚而不影响同一 I/O 端口的任何其他引脚。
提供以下 SWDIO I/O 引脚函数:
 - \ref PIN_SWDIO_OUT_ENABLE 用于从 DAP 硬件启用输出模式。
 - \ref PIN_SWDIO_OUT_DISABLE 用于启用到 DAP 硬件的输入模式。
 - \ref PIN_SWDIO_IN 用于以最快可能的速度从 SWDIO I/O 引脚读取。
 - \ref PIN_SWDIO_OUT 用于以最快可能的速度写入 SWDIO I/O 引脚。
*/


// 配置 DAP I/O 引脚 ------------------------------
#define PIN_SWDIO GPIO_NUM_8
#define PIN_SWCLK GPIO_NUM_9
#define PIN_nRESET GPIO_NUM_10
#define PIN_LED_CONNECTED GPIO_NUM_17
#define PIN_LED_RUNNING GPIO_NUM_18

/** 设置 JTAG I/O 引脚: TCK, TMS, TDI, TDO, nTRST 和 nRESET。
配置 JTAG 模式的 DAP 硬件 I/O 引脚:
 - TCK, TMS, TDI, nTRST, nRESET 设为输出模式并设为高电平。
 - TDO 设为输入模式。
*/
__STATIC_INLINE void PORT_JTAG_SETUP(void)
{
}

/** 设置 SWD I/O 引脚: SWCLK, SWDIO 和 nRESET。
配置串行线调试(SWD)模式的 DAP 硬件 I/O 引脚:
 - SWCLK, SWDIO, nRESET 设为输出模式并设为默认高电平。
 - TDI, TMS, nTRST 设为高阻态模式(SWD 模式下不使用这些引脚)。
*/
__STATIC_INLINE void PORT_SWD_SETUP(void)
{
    gpio_pad_select_gpio(PIN_SWCLK);
	gpio_set_direction(PIN_SWCLK, GPIO_MODE_INPUT_OUTPUT);
	gpio_pad_select_gpio(PIN_SWDIO);
	gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT_OUTPUT);

	gpio_set_level(PIN_SWCLK, 1);
	gpio_set_level(PIN_SWDIO, 1);
}

/** 禁用 JTAG/SWD I/O 引脚。
禁用 DAP 硬件 I/O 引脚,配置:
 - TCK/SWCLK, TMS/SWDIO, TDI, TDO, nTRST, nRESET 设为高阻态模式。
*/
__STATIC_INLINE void PORT_OFF(void)
{
	gpio_pad_select_gpio(PIN_SWCLK);
	gpio_set_direction(PIN_SWCLK, GPIO_MODE_INPUT);
	gpio_set_level(PIN_SWCLK, 0);
	gpio_pad_select_gpio(PIN_SWDIO);
	gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT);
	gpio_set_level(PIN_SWDIO, 0);
}


// SWCLK/TCK I/O 引脚 -------------------------------------

/** SWCLK/TCK I/O 引脚: 获取输入。
\return SWCLK/TCK DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN(void)
{
    return (uint32_t)gpio_get_level(PIN_SWCLK);
}

/** SWCLK/TCK I/O 引脚: 设置输出为高电平。
将 SWCLK/TCK DAP 硬件 I/O 引脚设为高电平。
*/
__STATIC_FORCEINLINE void     PIN_SWCLK_TCK_SET(void)
{
    WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (0x1 << PIN_SWCLK));
}

/** SWCLK/TCK I/O 引脚: 设置输出为低电平。
将 SWCLK/TCK DAP 硬件 I/O 引脚设为低电平。
*/
__STATIC_FORCEINLINE void     PIN_SWCLK_TCK_CLR(void)
{
    WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (0x1 << PIN_SWCLK));
}


// SWDIO/TMS 引脚 I/O --------------------------------------

/** SWDIO/TMS I/O 引脚: 获取输入。
\return SWDIO/TMS DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN(void)
{
    return gpio_get_level(PIN_SWDIO);
}

/** SWDIO/TMS I/O 引脚: 设置输出为高电平。
将 SWDIO/TMS DAP 硬件 I/O 引脚设为高电平。
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_TMS_SET(void)
{
    WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (0x1 << PIN_SWDIO));
}

/** SWDIO/TMS I/O 引脚: 设置输出为低电平。
将 SWDIO/TMS DAP 硬件 I/O 引脚设为低电平。
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_TMS_CLR(void)
{
    WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (0x1 << PIN_SWDIO));
}

/** SWDIO I/O 引脚: 获取输入(仅在 SWD 模式下使用)。
\return SWDIO DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN(void)
{
    return (uint32_t)gpio_get_level(PIN_SWDIO);
}

/** SWDIO I/O 引脚: 设置输出(仅在 SWD 模式下使用)。
\param bit SWDIO DAP 硬件 I/O 引脚的输出值。
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT(uint32_t bit)
{
    if ((bit & 1U) == 1)
	{
		WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (0x1 << PIN_SWDIO));
	}
	else
	{
		WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (0x1 << PIN_SWDIO));
	}
}

/** SWDIO I/O 引脚: 切换到输出模式(仅在 SWD 模式下使用)。
将 SWDIO DAP 硬件 I/O 引脚配置为输出模式。在调用 \ref PIN_SWDIO_OUT 函数之前调用此函数。
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT_ENABLE(void)
{
    gpio_set_direction(PIN_SWDIO, GPIO_MODE_OUTPUT);
}

/** SWDIO I/O 引脚: 切换到输入模式(仅在 SWD 模式下使用)。
将 SWDIO DAP 硬件 I/O 引脚配置为输入模式。在调用 \ref PIN_SWDIO_IN 函数之前调用此函数。
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT_DISABLE(void)
{
	gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT);
}


// TDI 引脚 I/O ---------------------------------------------

/** TDI I/O 引脚: 获取输入。
\return TDI DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void)
{
	return (0);   // 不可用
}

/** TDI I/O 引脚: 设置输出。
\param bit TDI DAP 硬件 I/O 引脚的输出值。
*/
__STATIC_FORCEINLINE void     PIN_TDI_OUT(uint32_t bit)
{
	(void)bit;   // 不可用
}


// TDO 引脚 I/O ---------------------------------------------

/** TDO I/O 引脚: 获取输入。
\return TDO DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void)
{
	return (0);   // 不可用
}


// nTRST 引脚 I/O -------------------------------------------

/** nTRST I/O 引脚: 获取输入。
\return nTRST DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void)
{
    return (0);   // 不可用
}

/** nTRST I/O 引脚: 设置输出。
\param bit JTAG TRST 测试复位引脚状态:
           - 0: 发出 JTAG TRST 测试复位。
           - 1: 释放 JTAG TRST 测试复位。
*/
__STATIC_FORCEINLINE void     PIN_nTRST_OUT(uint32_t bit)
{
    (void)bit;   // 不可用
}

// nRESET 引脚 I/O------------------------------------------

/** nRESET I/O 引脚: 获取输入。
\return nRESET DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void)
{
	return (uint32_t)gpio_get_level(PIN_nRESET);
}

/** nRESET I/O 引脚: 设置输出。
\param bit 目标设备硬件复位引脚状态:
           - 0: 发出设备硬件复位。
           - 1: 释放设备硬件复位。
*/

__STATIC_FORCEINLINE void     PIN_nRESET_OUT(uint32_t bit)
{
	if (bit)
	{
		WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (0x1 << PIN_nRESET));
	}
	else
	{
		WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (0x1 << PIN_nRESET));
	}
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_LEDs_gr CMSIS-DAP 硬件状态 LED
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件可以提供指示 CMSIS-DAP 调试单元状态的 LED。

建议提供以下 LED 用于状态指示:
 - 连接 LED: 当 DAP 硬件连接到调试器时点亮。
 - 运行 LED: 当调试器将目标设备置于运行状态时点亮。
*/

/** 调试单元: 设置连接 LED 的状态。
\param bit 连接 LED 的状态。
           - 1: 连接 LED 亮: 调试器已连接到 CMSIS-DAP 调试单元。
           - 0: 连接 LED 灭: 调试器未连接到 CMSIS-DAP 调试单元。
*/
__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit)
{
	if (bit)
	{
		WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (0x1 << PIN_LED_CONNECTED));
	}
	else
	{
		WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (0x1 << PIN_LED_CONNECTED));
	}
}

/** 调试单元: 设置目标运行 LED 的状态。
\param bit 目标运行 LED 的状态。
           - 1: 目标运行 LED 亮: 目标中的程序执行已启动。
           - 0: 目标运行 LED 灭: 目标中的程序执行已停止。
*/
__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit)
{
    if (bit)
	{
		WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (0x1 << PIN_LED_RUNNING));
	}
	else
	{
		WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (0x1 << PIN_LED_RUNNING));
	}
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_Timestamp_gr CMSIS-DAP 时间戳
\ingroup DAP_ConfigIO_gr
@{
测试域定时器的访问函数。

调试单元中测试域定时器的值由函数 \ref TIMESTAMP_GET 返回。
默认情况下使用 DWT 定时器。此定时器的频率通过 \ref TIMESTAMP_CLOCK 配置。

*/

/** 获取测试域定时器的时间戳。
\return 当前时间戳值。
*/
__STATIC_INLINE uint32_t TIMESTAMP_GET (void) {
  return xTaskGetTickCount();
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_Initialization_gr CMSIS-DAP 初始化
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件 I/O 和 LED 引脚通过函数 \ref DAP_SETUP 初始化。
*/

/** 设置调试单元 I/O 引脚和 LED(在初始化调试单元时调用)。
此函数执行 CMSIS-DAP 硬件 I/O 引脚和状态 LED 的初始化。
详细来说,启用并设置硬件 I/O 和 LED 引脚的操作:
 - 启用 I/O 时钟系统。
 - 所有 I/O 引脚: 启用输入缓冲区,输出引脚设为高阻态模式。
 - 对于 nTRST、nRESET,启用弱上拉(如果可用)。
 - 启用 LED 输出引脚并关闭 LED。
*/
__STATIC_INLINE void DAP_SETUP(void)
{
    PORT_JTAG_SETUP();
	PORT_SWD_SETUP();
	gpio_set_direction(PIN_nRESET, GPIO_MODE_INPUT_OUTPUT);
	gpio_set_level(PIN_nRESET, 1);
	// 配置: LED 为输出(关闭)
	gpio_set_direction(PIN_LED_CONNECTED, GPIO_MODE_OUTPUT);
	LED_CONNECTED_OUT(0);
	gpio_set_direction(PIN_LED_RUNNING, GPIO_MODE_OUTPUT);
	LED_RUNNING_OUT(0);
}

/** 使用自定义特定 I/O 引脚或命令序列复位目标设备。
此函数允许可选实现设备特定的复位序列。
当命令 \ref DAP_ResetTarget 调用时调用此函数,例如当设备需要时间关键的解锁序列来启用调试端口时需要此函数。
Reset Target Device with custom specific I/O pin or command sequence.
This function allows the optional implementation of a device specific reset sequence.
It is called when the command \ref DAP_ResetTarget and is for example required
when a device needs a time-critical unlock sequence that enables the debug port.
\return 0 = no device specific reset sequence is implemented.\n
        1 = a device specific reset sequence is implemented.
*/
__STATIC_INLINE uint32_t RESET_TARGET(void)
{
    return (0);              // change to '1' when a device reset sequence is implemented
}

///@}


#endif /* __DAP_CONFIG_H__ */
