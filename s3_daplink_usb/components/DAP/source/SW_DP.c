/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        SW_DP.c CMSIS-DAP SW DP I/O
 *
 *---------------------------------------------------------------------------*/

/**
 * @file SW_DP.c
 * @brief SWD 调试端口底层实现 - GPIO 和 SPI 双模式支持
 * 
 * 本文件实现了 ARM Serial Wire Debug (SWD) 协议的底层通信，包括：
 * 
 * 1. SWJ 序列 (SWJ_Sequence)
 *    - 用于 SWD/JTAG 模式切换
 *    - 发送特定位序列来选择调试协议
 * 
 * 2. SWD 序列 (SWD_Sequence)
 *    - 通用的 SWD 位序列读写
 *    - 支持任意长度（1-64位）的数据传输
 * 
 * 3. SWD 传输 (SWD_Transfer)
 *    - 完整的 SWD 读写事务
 *    - 包含请求包、ACK、数据和校验
 * 
 * 传输模式：
 * - GPIO 模式：使用 GPIO 引脚模拟 SWD 时序（兼容性好，速度较慢）
 * - SPI 模式：使用 SPI 外设加速传输（速度快，需要硬件支持）
 * 
 * SWD 协议简介：
 * - SWCLK: 时钟信号，由调试器产生
 * - SWDIO: 双向数据线，在时钟上升沿采样
 * - 数据传输采用 LSB 优先
 * 
 * @author windowsair
 * @change:
 *    2021-2-10 Support GPIO and SPI for SWD sequence / SWJ sequence / SWD transfer
 *              Note: SWD sequence not yet tested
 * @version 0.1
 * @date 2021-2-10
 *
 * @copyright Copyright (c) 2021
 *
 */

// ==========================================================================
// 头文件包含
// ==========================================================================

#include <stdio.h>

#include "DAP_config.h"   /* DAP 硬件配置（引脚定义、时钟等）*/
#include "DAP.h"          /* DAP 核心定义和数据结构 */
#include "spi_op.h"       /* SPI 操作函数 */
#include "spi_switch.h"   /* SPI/GPIO 模式切换 */
#include "dap_utility.h"  /* 工具函数（奇偶校验等）*/

// ==========================================================================
// 编译器优化宏
// ==========================================================================

/**
 * likely/unlikely 分支预测提示
 * 告诉编译器哪个分支更可能执行，优化指令流水线
 * 注意：ESP-IDF 的 esp_compiler.h 可能已定义这些宏，需要先取消再重新定义
 */
#ifdef likely
#undef likely
#endif
#ifdef unlikely
#undef unlikely
#endif
#define likely(x)    __builtin_expect(!!(x), 1)   /* x 很可能为真 */
#define unlikely(x)  __builtin_expect(!!(x), 0)   /* x 很可能为假 */

/**
 * 循环展开宏
 * 用于展开固定次数的循环，减少循环开销，提高执行速度
 * 在 SWD 32 位数据传输中使用 UNROLL_32 展开循环
 */
#define UNROLL_2(x)  x x
#define UNROLL_4(x)  UNROLL_2(x) UNROLL_2(x)
#define UNROLL_8(x)  UNROLL_4(x) UNROLL_4(x)
#define UNROLL_16(x) UNROLL_8(x) UNROLL_8(x)
#define UNROLL_32(x) UNROLL_16(x) UNROLL_16(x)

// ==========================================================================
// 调试配置
// ==========================================================================

/**
 * SWD 协议调试开关
 * 设为 1 时打印详细的 SWD 协议交互信息
 * 生产环境应设为 0
 */
#define PRINT_SWD_PROTOCOL 0

// ==========================================================================
// SWD 时序宏定义
// ==========================================================================

/**
 * 时钟延时宏
 * 根据配置的时钟延时值插入延时，控制 SWD 时钟频率
 */
#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

/**
 * 时钟引脚操作宏
 * SWCLK 和 TCK 共用同一引脚（SWD/JTAG 复用）
 */
#define PIN_SWCLK_SET PIN_SWCLK_TCK_SET  /* 时钟拉高 */
#define PIN_SWCLK_CLR PIN_SWCLK_TCK_CLR  /* 时钟拉低 */

/**
 * SWD 时钟周期宏
 * 
 * 产生一个完整的 SWCLK 时钟周期：
 * 1. 拉低时钟
 * 2. 延时（如果需要）
 * 3. 拉高时钟
 * 4. 延时（如果需要）
 * 
 * need_delay 参数控制是否插入延时：
 * - 1: 正常模式，插入延时以满足目标芯片时序要求
 * - 0: 快速模式，最小延时以获得最高速度
 */
#define SW_CLOCK_CYCLE()                     \
  PIN_SWCLK_CLR();                           \
  if (unlikely(need_delay)) { PIN_DELAY(); } \
  else { PIN_DELAY_FAST(); }                 \
  PIN_SWCLK_SET();                           \
  if (unlikely(need_delay)) { PIN_DELAY(); }

/**
 * SWD 写位宏
 * 
 * 在 SWDIO 上写入一个位：
 * 1. 设置 SWDIO 数据
 * 2. 拉低时钟（目标在此时采样数据）
 * 3. 延时
 * 4. 拉高时钟
 * 5. 延时
 * 
 * @param bit 要写入的位值（0 或 1）
 */
#define SW_WRITE_BIT(bit)                    \
  PIN_SWDIO_OUT(bit);                        \
  PIN_SWCLK_CLR();                           \
  if (unlikely(need_delay)) { PIN_DELAY(); } \
  PIN_SWCLK_SET();                           \
  if (unlikely(need_delay)) { PIN_DELAY(); }

/**
 * SWD 读位宏
 * 
 * 从 SWDIO 读取一个位：
 * 1. 拉低时钟
 * 2. 延时
 * 3. 采样 SWDIO 数据
 * 4. 拉高时钟
 * 5. 延时
 * 
 * @param bit 存储读取的位值
 */
#define SW_READ_BIT(bit)                     \
  PIN_SWCLK_CLR();                           \
  if (unlikely(need_delay)) { PIN_DELAY(); } \
  else { PIN_DELAY_FAST(); }                 \
  bit = PIN_SWDIO_IN();                      \
  PIN_SWCLK_SET();                           \
  if (unlikely(need_delay)) { PIN_DELAY(); }

// ==========================================================================
// 全局变量
// ==========================================================================

/**
 * SWD 传输速度模式
 * 
 * 可选值：
 * - kTransfer_SPI: 使用 SPI 外设加速传输
 * - kTransfer_GPIO_fast: GPIO 快速模式（最小延时）
 * - kTransfer_GPIO_normal: GPIO 正常模式（带延时）
 */
uint8_t SWD_TransferSpeed = kTransfer_GPIO_normal;

// ==========================================================================
// 函数前向声明
// ==========================================================================

/* GPIO 模式的 SWJ 序列函数 */
void SWJ_Sequence_GPIO (uint32_t count, const uint8_t *data, uint8_t need_delay);
/* SPI 模式的 SWJ 序列函数 */
void SWJ_Sequence_SPI (uint32_t count, const uint8_t *data);

/* GPIO 模式的 SWD 序列函数 */
void SWD_Sequence_GPIO (uint32_t info, const uint8_t *swdo, uint8_t *swdi);
/* SPI 模式的 SWD 序列函数 */
void SWD_Sequence_SPI (uint32_t info, const uint8_t *swdo, uint8_t *swdi);

// ==========================================================================
// SWJ 序列函数
// 
// SWJ (Serial Wire / JTAG) 序列用于：
// 1. SWD/JTAG 协议切换
// 2. 发送线路复位序列
// 3. 发送 JTAG-to-SWD 切换序列
// ==========================================================================

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))

/**
 * @brief 生成 SWJ 序列
 * 
 * 根据当前传输模式选择 GPIO 或 SPI 方式发送位序列。
 * 
 * 常用序列：
 * - 线路复位: 50+ 个 1，然后 2 个 0
 * - JTAG-to-SWD: 0xE79E (16位)
 * - SWD-to-JTAG: 0xE73C (16位)
 * 
 * @param count 要发送的位数
 * @param data  位数据指针（LSB 优先）
 */
void SWJ_Sequence (uint32_t count, const uint8_t *data) {
  if(SWD_TransferSpeed == kTransfer_SPI) {
    /* SPI 模式：使用 SPI 外设发送 */
    SWJ_Sequence_SPI(count, data);
  } else {
    /* GPIO 模式：使用 GPIO 模拟时序 */
    SWJ_Sequence_GPIO(count, data, 1);
  }
}

/**
 * @brief GPIO 模式的 SWJ 序列实现
 * 
 * 使用 GPIO 引脚逐位发送数据，每个位伴随一个时钟周期。
 * 
 * 时序：
 *   SWDIO: ----[D0]----[D1]----[D2]----...
 *   SWCLK: __/‾‾\__/‾‾\__/‾‾\__...
 * 
 * @param count      要发送的位数
 * @param data       位数据指针（LSB 优先，每字节 8 位）
 * @param need_delay 是否需要延时（1=正常模式，0=快速模式）
 * 
 * @note IRAM_ATTR 将函数放入 IRAM 以提高执行速度
 */
void IRAM_ATTR SWJ_Sequence_GPIO (uint32_t count, const uint8_t *data, uint8_t need_delay) {
    uint32_t val;  /* 当前字节数据 */
    uint32_t n;    /* 当前字节剩余位数 */

    val = 0U;
    n = 0U;
    
    /* 逐位发送数据 */
    while (count--) {
      /* 每 8 位加载新的字节 */
      if (n == 0U) {
        val = *data++;
        n = 8U;
      }
      
      /* 设置 SWDIO 输出（LSB 优先）*/
      if (val & 1U) {
        PIN_SWDIO_TMS_SET();  /* 输出 1 */
      } else {
        PIN_SWDIO_TMS_CLR();  /* 输出 0 */
      }
      
      /* 产生时钟周期 */
      SW_CLOCK_CYCLE();
      
      /* 移位到下一位 */
      val >>= 1;
      n--;
    }
}

/**
 * @brief SPI 模式的 SWJ 序列实现
 * 
 * 使用 SPI 外设发送位序列，比 GPIO 模式更快。
 * SPI MOSI 连接到 SWDIO，SPI CLK 连接到 SWCLK。
 * 
 * @param count 要发送的位数
 * @param data  位数据指针（LSB 优先）
 */
void SWJ_Sequence_SPI (uint32_t count, const uint8_t *data) {
  DAP_SPI_WriteBits(count, data);
}

#endif /* (DAP_SWD != 0) || (DAP_JTAG != 0) */

// ==========================================================================
// SWD 序列函数
// 
// SWD 序列用于通用的位级读写操作，支持 1-64 位的数据传输。
// 与 SWD_Transfer 不同，SWD_Sequence 不包含协议层（请求/ACK/校验）。
// ==========================================================================

#if (DAP_SWD != 0)

/**
 * @brief 生成 SWD 序列
 * 
 * 执行通用的 SWD 位序列读写操作。
 * 
 * @param info 序列信息，包含：
 *             - bit[5:0]: 时钟周期数（0 表示 64）
 *             - bit[7]: 方向（0=输出，1=输入）
 * @param swdo 输出数据指针（当 info 指定输出时使用）
 * @param swdi 输入数据指针（当 info 指定输入时使用）
 */
void SWD_Sequence (uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
  if (SWD_TransferSpeed == kTransfer_SPI) {
    SWD_Sequence_SPI(info, swdo, swdi);
  } else {
    SWD_Sequence_GPIO(info, swdo, swdi);
  }
}

/**
 * @brief GPIO 模式的 SWD 序列实现
 * 
 * 使用 GPIO 引脚执行位级读写操作。
 * 
 * @param info 序列信息
 * @param swdo 输出数据指针
 * @param swdi 输入数据指针
 * 
 * @note IRAM_ATTR 将函数放入 IRAM 以提高执行速度
 */
void IRAM_ATTR SWD_Sequence_GPIO (uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
  const uint8_t need_delay = 1;  /* GPIO 模式使用延时 */

  uint32_t val;  /* 数据值 */
  uint32_t bit;  /* 单个位 */
  uint32_t n, k; /* 计数器 */

  /* 提取时钟周期数（0 表示 64）*/
  n = info & SWD_SEQUENCE_CLK;
  if (n == 0U) {
    n = 64U;
  }
  /* n = 1 ~ 64 */

  /* 根据方向执行读或写操作（LSB 优先）*/
  if (info & SWD_SEQUENCE_DIN) {
    /* 输入模式：从 SWDIO 读取数据 */
    while (n) {
      val = 0U;
      /* 每次最多读取 8 位 */
      for (k = 8U; k && n; k--, n--) {
        SW_READ_BIT(bit);
        val >>= 1;
        val  |= bit << 7;  /* LSB 优先，从高位插入 */
      }
      val >>= k;  /* 对齐到 LSB */
      *swdi++ = (uint8_t)val;
    }
  } else {
    /* 输出模式：向 SWDIO 写入数据 */
    while (n) {
      val = *swdo++;
      /* 每次最多写入 8 位 */
      for (k = 8U; k && n; k--, n--) {
        SW_WRITE_BIT(val);
        val >>= 1;  /* LSB 优先 */
      }
    }
  }
}

/**
 * @brief SPI 模式的 SWD 序列实现
 * 
 * 使用 SPI 外设执行位级读写操作。
 * 
 * @param info 序列信息
 * @param swdo 输出数据指针
 * @param swdi 输入数据指针
 */
void SWD_Sequence_SPI (uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
  uint32_t n;
  
  /* 提取时钟周期数（0 表示 64）*/
  n = info & SWD_SEQUENCE_CLK;
  if (n == 0U) {
    n = 64U;
  }
  /* n = 1 ~ 64 */

  if (info & SWD_SEQUENCE_DIN) {
    /* 输入模式：使用 SPI 读取 */
    DAP_SPI_ReadBits(n, swdi);
  } else {
    /* 输出模式：使用 SPI 写入 */
    DAP_SPI_WriteBits(n, swdo);
  }
}

#endif /* DAP_SWD != 0 */

// ==========================================================================
// SWD 传输函数
// 
// SWD 传输是完整的 SWD 协议事务，包含：
// 1. 请求包 (8位): Start + APnDP + RnW + A[3:2] + Parity + Stop + Park
// 2. 转向周期 (Turnaround)
// 3. 应答 (3位): ACK[2:0]
// 4. 数据 (33位): DATA[31:0] + Parity
// 5. 空闲周期 (可选)
// 
// ACK 响应：
// - OK (0b001): 传输成功
// - WAIT (0b010): 目标忙，需要重试
// - FAULT (0b100): 传输错误
// ==========================================================================

#if (DAP_SWD != 0)

/**
 * @brief SPI 模式的 SWD 传输实现
 * 
 * 使用 SPI 外设执行完整的 SWD 读写事务。
 * SPI 模式比 GPIO 模式更快，但需要硬件支持。
 * 
 * SWD 请求包格式（8位，LSB 优先）：
 *   bit[0]: Start (固定为 1)
 *   bit[1]: APnDP (0=DP, 1=AP)
 *   bit[2]: RnW (0=写, 1=读)
 *   bit[3]: A2 (地址位 2)
 *   bit[4]: A3 (地址位 3)
 *   bit[5]: Parity (奇偶校验)
 *   bit[6]: Stop (固定为 0)
 *   bit[7]: Park (固定为 1)
 * 
 * @param request 请求参数：
 *                - bit[0]: APnDP
 *                - bit[1]: RnW
 *                - bit[2]: A2
 *                - bit[3]: A3
 * @param data    数据指针（读操作时存储读取的数据，写操作时包含要写入的数据）
 * @return ACK 响应（DAP_TRANSFER_OK/WAIT/FAULT/ERROR）
 */
static uint8_t SWD_Transfer_SPI (uint32_t request, uint32_t *data) {
  /* 注意：SPI 模式不需要 PIN_DELAY 操作 */
  uint8_t ack;                /* ACK 响应 */
  uint32_t val;               /* 数据值 */
  uint8_t parity;             /* 接收的奇偶校验位 */
  uint8_t computedParity;     /* 计算的奇偶校验 */
  uint32_t n;                 /* 计数器 */

  /* 请求包常量位：Start=1, Stop=0, Park=1 -> 0b10000001 */
  const uint8_t constantBits = 0b10000001U;
  uint8_t requestByte;  /* 完整的请求字节（LSB 优先）*/

  /* 构造请求字节：
   * - bit[0]: Start = 1
   * - bit[1-4]: APnDP, RnW, A2, A3 (从 request 参数)
   * - bit[5]: Parity (4位的偶校验)
   * - bit[6]: Stop = 0
   * - bit[7]: Park = 1
   */
  requestByte = constantBits | (((uint8_t)(request & 0xFU)) << 1U) | (ParityEvenUint8(request & 0xFU) << 5U);

#if (PRINT_SWD_PROTOCOL == 1)
  switch (requestByte)
    {
    case 0xA5U:
      os_printf("IDCODE\r\n");
      break;
    case 0xA9U:
      os_printf("W CTRL/STAT\r\n");
      break;
    case 0xBDU:
      os_printf("RDBUFF\r\n");
      break;
    case 0x8DU:
      os_printf("R CTRL/STAT\r\n");
      break;
    case 0x81U:
      os_printf("W ABORT\r\n");
      break;
    case 0xB1U:
      os_printf("W SELECT\r\n");
      break;
    case 0xBBU:
      os_printf("W APc\r\n");
      break;
    case 0x9FU:
      os_printf("R APc\r\n");
      break;
    case 0x8BU:
      os_printf("W AP4\r\n");
      break;
    case 0xA3U:
      os_printf("W AP0\r\n");
      break;
    case 0X87U:
      os_printf("R AP0\r\n");
      break;
    case 0xB7U:
      os_printf("R AP8\r\n");
      break;
    default:
    //W AP8
      os_printf("Unknown:%08x\r\n", requestByte);
      break;
    }
#endif

  /* ==================== 读操作 ==================== */
  if (request & DAP_TRANSFER_RnW) {
    /* 
     * SWD 读事务时序：
     * 主机 -> 目标: [请求包 8位] [Trn]
     * 目标 -> 主机: [ACK 3位] [数据 32位] [校验 1位] [Trn]
     */

    /* 发送请求头，0 表示 ACK 后不需要额外转向周期 */
    DAP_SPI_Send_Header(requestByte, &ack, 0);
    
    if (ack == DAP_TRANSFER_OK) {
      /* ACK=OK: 读取 32 位数据和校验位 */
      DAP_SPI_Read_Data(&val, &parity);
      computedParity = ParityEvenUint32(val);

      /* 验证奇偶校验 */
      if ((computedParity ^ parity) & 1U) {
        ack = DAP_TRANSFER_ERROR;  /* 校验错误 */
      }
      if (data) { *data = val; }

      /* 捕获时间戳（如果请求）*/
      if (request & DAP_TRANSFER_TIMESTAMP) {
        DAP_Data.timestamp = TIMESTAMP_GET();
      }
    }
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
      /* ACK=WAIT/FAULT: 目标忙或出错，产生额外时钟周期 */
#if defined CONFIG_IDF_TARGET_ESP8266 || defined CONFIG_IDF_TARGET_ESP32
      DAP_SPI_Generate_Cycle(1);
#elif defined CONFIG_IDF_TARGET_ESP32C3 || defined CONFIG_IDF_TARGET_ESP32S3
      DAP_SPI_Fast_Cycle();
#endif

#if (PRINT_SWD_PROTOCOL == 1)
      os_printf("WAIT\r\n");
#endif
    }
    else {
      /* 协议错误：ACK 无效，执行错误恢复 */
      PIN_SWDIO_TMS_SET();
      DAP_SPI_Protocol_Error_Read();
      PIN_SWDIO_TMS_SET();
#if (PRINT_SWD_PROTOCOL == 1)
      os_printf("Protocol Error: Read\r\n");
#endif
    }

    return ((uint8_t)ack);
  }
  /* ==================== 写操作 ==================== */
  else {
    /* 
     * SWD 写事务时序：
     * 主机 -> 目标: [请求包 8位] [Trn]
     * 目标 -> 主机: [ACK 3位] [Trn]
     * 主机 -> 目标: [数据 32位] [校验 1位]
     */
    
    /* 计算数据的奇偶校验 */
    parity = ParityEvenUint32(*data);
    
    /* 发送请求头，1 表示 ACK 后需要转向周期（切换数据方向）*/
    DAP_SPI_Send_Header(requestByte, &ack, 1);
    
    if (ack == DAP_TRANSFER_OK) {
      /* ACK=OK: 写入 32 位数据和校验位 */
      DAP_SPI_Write_Data(*data, parity);
      
      /* 捕获时间戳（如果请求）*/
      if (request & DAP_TRANSFER_TIMESTAMP) {
        DAP_Data.timestamp = TIMESTAMP_GET();
      }
      
      /* 空闲周期：某些目标需要额外的时钟周期来处理写入 */
      n = DAP_Data.transfer.idle_cycles;
      if (n) { DAP_SPI_Generate_Cycle(n); }

      /* 恢复 SWDIO 为高电平（空闲状态）*/
      PIN_SWDIO_TMS_SET();

      return ((uint8_t)ack);
    }
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
      /* ACK=WAIT/FAULT: 目标忙或出错
       * 此时已经完成了转向周期，不需要额外操作 */
#if (PRINT_SWD_PROTOCOL == 1)
      os_printf("WAIT\r\n");
#endif
    }
    else {
      /* 协议错误：ACK 无效，执行错误恢复 */
      PIN_SWDIO_TMS_SET();
      DAP_SPI_Protocol_Error_Write();
      PIN_SWDIO_TMS_SET();
#if (PRINT_SWD_PROTOCOL == 1)
      os_printf("Protocol Error: Write\r\n");
#endif
    }

    return ((uint8_t)ack);
  }

  /* 不应该到达这里 */
  return DAP_TRANSFER_ERROR;
}

/**
 * @brief GPIO 模式的 SWD 传输实现
 * 
 * 使用 GPIO 引脚模拟 SWD 时序，执行完整的读写事务。
 * GPIO 模式兼容性好，但速度比 SPI 模式慢。
 * 
 * @param request    请求参数（APnDP, RnW, A[3:2]）
 * @param data       数据指针
 * @param need_delay 是否需要延时（1=正常模式，0=快速模式）
 * @return ACK 响应
 * 
 * @note IRAM_ATTR 将函数放入 IRAM 以提高执行速度
 */
static uint8_t IRAM_ATTR SWD_Transfer_GPIO (uint32_t request, uint32_t *data, uint8_t need_delay) {
  uint32_t ack;     /* ACK 响应 */
  uint32_t bit;     /* 单个位 */
  uint32_t val;     /* 数据值 */
  uint32_t parity;  /* 奇偶校验 */
  uint32_t n;       /* 计数器 */

  /* ==================== 发送请求包 (8位) ==================== */
  /* 
   * 请求包格式（LSB 优先）：
   * [Start=1] [APnDP] [RnW] [A2] [A3] [Parity] [Stop=0] [Park=1]
   */
  parity = 0U;
  SW_WRITE_BIT(1U);                     /* Start Bit: 固定为 1 */
  bit = request >> 0;
  SW_WRITE_BIT(bit);                    /* APnDP Bit: 0=DP, 1=AP */
  parity += bit;
  bit = request >> 1;
  SW_WRITE_BIT(bit);                    /* RnW Bit: 0=写, 1=读 */
  parity += bit;
  bit = request >> 2;
  SW_WRITE_BIT(bit);                    /* A2 Bit */
  parity += bit;
  bit = request >> 3;
  SW_WRITE_BIT(bit);                    /* A3 Bit */
  parity += bit;
  SW_WRITE_BIT(parity);                 /* Parity Bit: 偶校验 */
  SW_WRITE_BIT(0U);                     /* Stop Bit: 固定为 0 */
  SW_WRITE_BIT(1U);                     /* Park Bit: 固定为 1 */

  /* ==================== 转向周期 (Turnaround) ==================== */
  /* 
   * 转向周期用于切换 SWDIO 数据方向
   * 主机释放 SWDIO，等待目标驱动
   */
  PIN_SWDIO_OUT_DISABLE();  /* 将 SWDIO 设为输入模式 */
  for (n = DAP_Data.swd_conf.turnaround; n; n--) {
    SW_CLOCK_CYCLE();
  }

  /* ==================== 读取 ACK 响应 (3位) ==================== */
  /* 
   * ACK 响应由目标发送：
   * - OK (0b001): 传输成功
   * - WAIT (0b010): 目标忙
   * - FAULT (0b100): 传输错误
   */
  SW_READ_BIT(bit);
  ack  = bit << 0;
  SW_READ_BIT(bit);
  ack |= bit << 1;
  SW_READ_BIT(bit);
  ack |= bit << 2;

  /* ==================== ACK=OK: 执行数据传输 ==================== */
  if (ack == DAP_TRANSFER_OK) {
    if (request & DAP_TRANSFER_RnW) {
      /* ---------- 读操作 ---------- */
      /* 从目标读取 32 位数据 + 1 位校验 */
      val = 0U;
      parity = 0U;
      UNROLL_32({
        SW_READ_BIT(bit);               /* 读取 RDATA[0:31] */
        parity += bit;                  /* 累加校验 */
        val >>= 1;
        val  |= bit << 31;              /* LSB 优先，从高位插入 */
      })
      SW_READ_BIT(bit);                 /* 读取校验位 */
      if ((parity ^ bit) & 1U) {
        ack = DAP_TRANSFER_ERROR;       /* 校验错误 */
      }
      if (data) { *data = val; }
      
      /* 转向周期：切换回主机驱动 */
      for (n = DAP_Data.swd_conf.turnaround; n; n--) {
        SW_CLOCK_CYCLE();
      }
      PIN_SWDIO_OUT_ENABLE();           /* 将 SWDIO 设为输出模式 */
    } else {
      /* ---------- 写操作 ---------- */
      /* 转向周期：切换回主机驱动 */
      for (n = DAP_Data.swd_conf.turnaround; n; n--) {
        SW_CLOCK_CYCLE();
      }
      PIN_SWDIO_OUT_ENABLE();           /* 将 SWDIO 设为输出模式 */
      
      /* 向目标写入 32 位数据 + 1 位校验 */
      val = *data;
      parity = 0U;
      UNROLL_32({
        SW_WRITE_BIT(val);              /* 写入 WDATA[0:31] */
        parity += val;                  /* 累加校验 */
        val >>= 1;                      /* LSB 优先 */
      })
      SW_WRITE_BIT(parity);             /* 写入校验位 */
    }
    
    /* 捕获时间戳（如果请求）*/
    if (request & DAP_TRANSFER_TIMESTAMP) {
      DAP_Data.timestamp = TIMESTAMP_GET();
    }
    
    /* 空闲周期：某些目标需要额外时钟周期 */
    n = DAP_Data.transfer.idle_cycles;
    if (n) {
      PIN_SWDIO_OUT(0U);                /* 空闲时 SWDIO 为低 */
      for (; n; n--) {
        SW_CLOCK_CYCLE();
      }
    }
    PIN_SWDIO_OUT(1U);                  /* 恢复 SWDIO 为高（空闲状态）*/
    return ((uint8_t)ack);
  }

  /* ==================== ACK=WAIT/FAULT: 目标忙或出错 ==================== */
  if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
    /* 
     * 如果配置了 data_phase，即使 ACK 不是 OK，也需要完成数据阶段
     * 这是为了保持协议同步
     */
    if (DAP_Data.swd_conf.data_phase && ((request & DAP_TRANSFER_RnW) != 0U)) {
      /* 读操作：产生 33 个空时钟周期（32位数据 + 1位校验）*/
      for (n = 32U+1U; n; n--) {
        SW_CLOCK_CYCLE();               /* 空读 RDATA[0:31] + Parity */
      }
    }
    
    /* 转向周期 */
    for (n = DAP_Data.swd_conf.turnaround; n; n--) {
      SW_CLOCK_CYCLE();
    }
    PIN_SWDIO_OUT_ENABLE();
    
    if (DAP_Data.swd_conf.data_phase && ((request & DAP_TRANSFER_RnW) == 0U)) {
      /* 写操作：产生 33 个空时钟周期（32位数据 + 1位校验）*/
      PIN_SWDIO_OUT(0U);
      for (n = 32U+1U; n; n--) {
        SW_CLOCK_CYCLE();               /* 空写 WDATA[0:31] + Parity */
      }
    }
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }

  /* ==================== 协议错误：ACK 无效 ==================== */
  /* 
   * 收到无效的 ACK（不是 OK/WAIT/FAULT）
   * 需要产生足够的时钟周期来恢复协议同步
   */
  for (n = DAP_Data.swd_conf.turnaround + 32U + 1U; n; n--) {
    SW_CLOCK_CYCLE();                   /* 退出数据阶段 */
  }
  PIN_SWDIO_OUT_ENABLE();
  PIN_SWDIO_OUT(1U);                    /* 恢复空闲状态 */
  return ((uint8_t)ack);
}

/**
 * @brief SWD 传输入口函数
 * 
 * 根据当前配置的传输速度模式，选择合适的实现函数。
 * 
 * @param request 请求参数：
 *                - bit[0]: APnDP (0=DP, 1=AP)
 *                - bit[1]: RnW (0=写, 1=读)
 *                - bit[2]: A2 (地址位 2)
 *                - bit[3]: A3 (地址位 3)
 * @param data    数据指针（读操作时存储结果，写操作时包含数据）
 * @return ACK 响应：
 *         - DAP_TRANSFER_OK (0x01): 成功
 *         - DAP_TRANSFER_WAIT (0x02): 目标忙
 *         - DAP_TRANSFER_FAULT (0x04): 错误
 *         - DAP_TRANSFER_ERROR (0x08): 协议错误
 */
uint8_t SWD_Transfer(uint32_t request, uint32_t *data) {
  switch (SWD_TransferSpeed) {
    case kTransfer_SPI:
      /* SPI 模式：最快，使用 SPI 外设 */
      return SWD_Transfer_SPI(request, data);
    case kTransfer_GPIO_fast:
      /* GPIO 快速模式：无延时 */
      return SWD_Transfer_GPIO(request, data, 0);
    case kTransfer_GPIO_normal:
      /* GPIO 正常模式：带延时 */
      return SWD_Transfer_GPIO(request, data, 1);
    default:
      /* 默认使用 GPIO 正常模式 */
      return SWD_Transfer_GPIO(request, data, 1);
  }
}

#endif  /* (DAP_SWD != 0) */
