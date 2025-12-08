/*
 * 版权所有 (c) 2013-2017 ARM Limited. 保留所有权利。
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * 根据Apache许可证2.0版（"许可证"）授权；除非遵守许可证，
 * 否则您不得使用此文件。您可以在以下位置获取许可证副本：
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * 除非适用法律要求或书面同意，否则根据许可证分发的软件
 * 均按"原样"分发，不附带任何明示或暗示的保证或条件。
 * 有关许可证下特定语言的权限和限制，请参阅许可证。
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        2021年6月16日
 * $Revision:    V2.1.0
 *
 * 项目:      CMSIS-DAP 源代码
 * 标题:      DAP.c CMSIS-DAP 命令处理
 *
 *---------------------------------------------------------------------------*/

#include <string.h>

#include "DAP_config.h"
#include "DAP.h"
#include "spi_switch.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 数据包大小检查 - 最小值为64字节 */
#if (DAP_PACKET_SIZE < 64U)
#error "最小数据包大小为64字节!"
#endif

/* 数据包大小检查 - 最大值为32768字节 */
#if (DAP_PACKET_SIZE > 32768U)
#error "最大数据包大小为32768字节!"
#endif

/* 数据包计数检查 - 最小值为1 */
#if (DAP_PACKET_COUNT < 1U)
#error "最小数据包计数为1!"
#endif

/* 数据包计数检查 - 最大值为255 */
#if (DAP_PACKET_COUNT > 255U)
#error "最大数据包计数为255!"
#endif


// 时钟宏定义

/**
 * @brief 计算给定延迟周期下的最大SWJ时钟频率
 * @param delay_cycles 延迟周期数
 * @return 最大SWJ时钟频率（Hz）
 * 
 * 公式说明：CPU时钟的一半除以（IO端口写入周期 + 延迟周期）
 */
#define MAX_SWJ_CLOCK(delay_cycles) \
  ((CPU_CLOCK/2U) / (IO_PORT_WRITE_CYCLES + delay_cycles))

/**
 * @brief 根据目标SWJ时钟频率计算所需的延迟周期
 * @param swj_clock 目标SWJ时钟频率（Hz）
 * @return 需要的延迟周期数
 * 
 * 公式说明：（CPU时钟的一半除以目标时钟）减去IO端口写入周期
 */
#define CLOCK_DELAY(swj_clock) \
 (((CPU_CLOCK/2U) / swj_clock) - IO_PORT_WRITE_CYCLES)


         DAP_Data_t DAP_Data;           // DAP数据结构，存储DAP配置和状态信息
volatile uint8_t    DAP_TransferAbort;  // 传输中止标志，用于取消正在进行的传输操作


/* DAP固件版本字符串 */
static const char DAP_FW_Ver [] = DAP_FW_VER;



/**
 * @brief 获取DAP信息
 * 
 * 根据请求的信息标识符返回相应的DAP信息数据
 * 
 * @param id    信息标识符，指定要获取的信息类型
 * @param info  指向信息数据缓冲区的指针，用于存储返回的信息
 * @return      信息数据的字节数
 */
static uint8_t DAP_Info(uint8_t id, uint8_t *info) {
  uint8_t length = 0U;  // 返回数据的长度

  switch (id) {
    case DAP_ID_VENDOR:
      /* 获取厂商字符串 */
      length = DAP_GetVendorString((char *)info);
      break;
    case DAP_ID_PRODUCT:
      /* 获取产品字符串 */
      length = DAP_GetProductString((char *)info);
      break;
    case DAP_ID_SER_NUM:
      /* 获取序列号字符串 */
      length = DAP_GetSerNumString((char *)info);
      break;
    case DAP_ID_DAP_FW_VER:
      /* 获取DAP固件版本 */
      length = (uint8_t)sizeof(DAP_FW_Ver);
      memcpy(info, DAP_FW_Ver, length);
      break;
    case DAP_ID_DEVICE_VENDOR:
      /* 获取目标设备厂商字符串 */
      length = DAP_GetTargetDeviceVendorString((char *)info);
      break;
    case DAP_ID_DEVICE_NAME:
      /* 获取目标设备名称字符串 */
      length = DAP_GetTargetDeviceNameString((char *)info);
      break;
    case DAP_ID_BOARD_VENDOR:
      /* 获取目标板厂商字符串 */
      length = DAP_GetTargetBoardVendorString((char *)info);
      break;
    case DAP_ID_BOARD_NAME:
      /* 获取目标板名称字符串 */
      length = DAP_GetTargetBoardNameString((char *)info);
      break;
    case DAP_ID_PRODUCT_FW_VER:
      /* 获取产品固件版本字符串 */
      length = DAP_GetProductFirmwareVersionString((char *)info);
      break;
    case DAP_ID_CAPABILITIES:
      /* 获取DAP功能/能力信息 */
      /* 第一个字节包含以下功能位：
       * Bit 0: SWD支持（串行线调试）
       * Bit 1: JTAG支持
       * Bit 2: SWO UART模式支持（串行线输出）
       * Bit 3: SWO Manchester编码支持
       * Bit 4: 原子命令支持
       * Bit 5: 时间戳时钟支持
       * Bit 6: SWO流模式支持
       * Bit 7: DAP UART支持
       */
      info[0] = ((DAP_SWD  != 0)         ? (1U << 0) : 0U) |
                ((DAP_JTAG != 0)         ? (1U << 1) : 0U) |
                ((SWO_UART != 0)         ? (1U << 2) : 0U) |
                ((SWO_MANCHESTER != 0)   ? (1U << 3) : 0U) |
                /* 原子命令支持 */         (1U << 4)       |
                ((TIMESTAMP_CLOCK != 0U) ? (1U << 5) : 0U) |
                ((SWO_STREAM != 0U)      ? (1U << 6) : 0U) |
                ((DAP_UART != 0U)        ? (1U << 7) : 0U);

      /* 第二个字节包含以下功能位：
       * Bit 0: UART USB COM端口支持
       */
      info[1] = ((DAP_UART_USB_COM_PORT != 0) ? (1U << 0) : 0U);
      length = 2U;
      break;
    case DAP_ID_TIMESTAMP_CLOCK:
      /* 获取时间戳时钟频率（4字节，小端序） */
#if (TIMESTAMP_CLOCK != 0U)
      info[0] = (uint8_t)(TIMESTAMP_CLOCK >>  0);  // 最低字节
      info[1] = (uint8_t)(TIMESTAMP_CLOCK >>  8);
      info[2] = (uint8_t)(TIMESTAMP_CLOCK >> 16);
      info[3] = (uint8_t)(TIMESTAMP_CLOCK >> 24);  // 最高字节
      length = 4U;
#endif
      break;
    case DAP_ID_UART_RX_BUFFER_SIZE:
      /* 获取UART接收缓冲区大小（4字节，小端序） */
#if (DAP_UART != 0)
      info[0] = (uint8_t)(DAP_UART_RX_BUFFER_SIZE >>  0);
      info[1] = (uint8_t)(DAP_UART_RX_BUFFER_SIZE >>  8);
      info[2] = (uint8_t)(DAP_UART_RX_BUFFER_SIZE >> 16);
      info[3] = (uint8_t)(DAP_UART_RX_BUFFER_SIZE >> 24);
      length = 4U;
#endif
      break;
    case DAP_ID_UART_TX_BUFFER_SIZE:
      /* 获取UART发送缓冲区大小（4字节，小端序） */
#if (DAP_UART != 0)
      info[0] = (uint8_t)(DAP_UART_TX_BUFFER_SIZE >>  0);
      info[1] = (uint8_t)(DAP_UART_TX_BUFFER_SIZE >>  8);
      info[2] = (uint8_t)(DAP_UART_TX_BUFFER_SIZE >> 16);
      info[3] = (uint8_t)(DAP_UART_TX_BUFFER_SIZE >> 24);
      length = 4U;
#endif
      break;
    case DAP_ID_SWO_BUFFER_SIZE:
      /* 获取SWO缓冲区大小（4字节，小端序） */
#if ((SWO_UART != 0) || (SWO_MANCHESTER != 0))
      info[0] = (uint8_t)(SWO_BUFFER_SIZE >>  0);
      info[1] = (uint8_t)(SWO_BUFFER_SIZE >>  8);
      info[2] = (uint8_t)(SWO_BUFFER_SIZE >> 16);
      info[3] = (uint8_t)(SWO_BUFFER_SIZE >> 24);
      length = 4U;
#endif
      break;
    case DAP_ID_PACKET_SIZE:
      /* 获取数据包大小（2字节，小端序） */
      info[0] = (uint8_t)(DAP_PACKET_SIZE >> 0);  // 低字节
      info[1] = (uint8_t)(DAP_PACKET_SIZE >> 8);  // 高字节
      length = 2U;
      break;
    case DAP_ID_PACKET_COUNT:
      /* 获取数据包计数（1字节） */
      info[0] = DAP_PACKET_COUNT;
      length = 1U;
      break;
    default:
      /* 未知的信息标识符，返回长度为0 */
      break;
  }

  return (length);
}

/**
 * @brief 延时指定的毫秒数
 * @param delay 延时时间（毫秒）
 * 
 * 将毫秒转换为延时循环次数，然后执行慢速延时
 * 计算公式：delay * (CPU时钟/1000 + 延时周期-1) / 延时周期
 */
void Delayms(uint32_t delay) {
  delay *= ((CPU_CLOCK/1000U) + (DELAY_SLOW_CYCLES-1U)) / DELAY_SLOW_CYCLES;
  PIN_DELAY_SLOW(delay);
}


/**
 * @brief 处理延时命令并准备响应
 * @param request  指向请求数据的指针
 * @param response 指向响应数据的指针
 * @return 响应字节数（低16位）| 请求字节数（高16位）
 * 
 * 请求格式：2字节延时值（微秒，小端序）
 * 响应格式：1字节状态（DAP_OK）
 */
static uint32_t DAP_Delay(const uint8_t *request, uint8_t *response) {
  uint32_t delay;

  /* 从请求中提取16位延时值（小端序） */
  delay  = (uint32_t)(*(request+0)) |
           (uint32_t)(*(request+1) << 8);
  
  /* 将微秒转换为延时循环次数 */
  delay *= ((CPU_CLOCK/1000000U) + (DELAY_SLOW_CYCLES-1U)) / DELAY_SLOW_CYCLES;

  /* 执行延时 */
  PIN_DELAY_SLOW(delay);

  *response = DAP_OK;
  return ((2U << 16) | 1U);
}


/**
 * @brief 处理主机状态命令并准备响应
 * @param request  指向请求数据的指针
 * @param response 指向响应数据的指针
 * @return 响应字节数（低16位）| 请求字节数（高16位）
 * 
 * 请求格式：
 *   字节0: 状态类型（DAP_DEBUGGER_CONNECTED 或 DAP_TARGET_RUNNING）
 *   字节1: 状态值（bit0有效，1=开启，0=关闭）
 * 响应格式：1字节状态（DAP_OK 或 DAP_ERROR）
 * 
 * 功能：控制调试器连接状态LED和目标运行状态LED
 */
static uint32_t DAP_HostStatus(const uint8_t *request, uint8_t *response) {

  switch (*request) {
    case DAP_DEBUGGER_CONNECTED:
      /* 设置调试器连接状态LED */
      LED_CONNECTED_OUT((*(request+1) & 1U));
      break;
    case DAP_TARGET_RUNNING:
      /* 设置目标运行状态LED */
      LED_RUNNING_OUT((*(request+1) & 1U));
      break;
    default:
      /* 未知状态类型，返回错误 */
      *response = DAP_ERROR;
      return ((2U << 16) | 1U);
  }

  *response = DAP_OK;
  return ((2U << 16) | 1U);
}


/**
 * @brief 处理连接命令并准备响应
 * @param request  指向请求数据的指针
 * @param response 指向响应数据的指针
 * @return 响应字节数（低16位）| 请求字节数（高16位）
 * 
 * 请求格式：1字节端口类型
 *   - DAP_PORT_AUTODETECT: 自动检测（使用默认端口）
 *   - DAP_PORT_SWD: SWD调试端口
 *   - DAP_PORT_JTAG: JTAG调试端口
 * 响应格式：1字节实际连接的端口类型
 * 
 * 功能：初始化指定的调试端口（SWD或JTAG）
 */
static uint32_t DAP_Connect(const uint8_t *request, uint8_t *response) {
  uint32_t port;

  /* 如果请求自动检测，则使用默认端口 */
  if (*request == DAP_PORT_AUTODETECT) {
    port = DAP_DEFAULT_PORT;
  } else {
    port = *request;
  }

  switch (port) {
#if (DAP_SWD != 0)
    case DAP_PORT_SWD:
      /* 配置SWD调试端口 */
      DAP_Data.debug_port = DAP_PORT_SWD;
      /* 如果当前不是SPI传输模式，则初始化SWD端口引脚 */
      if (SWD_TransferSpeed != kTransfer_SPI)
        PORT_SWD_SETUP();
      break;
#endif
#if (DAP_JTAG != 0)
    case DAP_PORT_JTAG:
      /* 配置JTAG调试端口 */
      DAP_Data.debug_port = DAP_PORT_JTAG;
      PORT_JTAG_SETUP();
      break;
#endif
    default:
      /* 不支持的端口类型，设置为禁用 */
      port = DAP_PORT_DISABLED;
      break;
  }

  /* 返回实际连接的端口类型 */
  *response = (uint8_t)port;
  return ((1U << 16) | 1U);
}


/*
 * 处理断开连接命令并准备响应
 * 
 * 参数:
 *   response: 指向响应数据的指针
 * 
 * 返回值:
 *   响应数据的字节数
 * 
 * 功能：禁用调试端口并关闭相关引脚
 */
static uint32_t DAP_Disconnect(uint8_t *response) {

  /* 将调试端口设置为禁用状态 */
  DAP_Data.debug_port = DAP_PORT_DISABLED;
  /* 关闭所有调试端口引脚 */
  PORT_OFF();

  /* 返回操作成功状态 */
  *response = DAP_OK;
  return (1U);
}


/*
 * 处理复位目标命令并准备响应
 * 
 * 参数:
 *   response: 指向响应数据的指针
 * 
 * 返回值:
 *   响应数据的字节数
 * 
 * 功能：执行目标芯片复位操作
 */
static uint32_t DAP_ResetTarget(uint8_t *response) {

#if (USE_FORCE_SYSRESETREQ_AFTER_FLASH)
  /* 当nRESET引脚未连接时的软件复位解决方案 */
  if (DAP_Data.debug_port == DAP_PORT_SWD) {
    uint8_t ack;
    /* AIRCR寄存器地址（应用中断和复位控制寄存器） */
    uint32_t AIRCR_REG_ADDR = 0xE000ED0C;
    /* AIRCR复位值：VECTKEY(0x05FA) | SYSRESETREQ位 */
    uint32_t AIRCR_RESET_VAL = 0x05FA << 16 | 1 << 2;
    /* 构建AP写请求：选择AP，地址位A2 */
    uint8_t req = DAP_TRANSFER_APnDP | 0 | DAP_TRANSFER_A2 | 0;
    /* 发送地址到TAR寄存器 */
    ack = SWD_Transfer(req, &AIRCR_REG_ADDR);
    if (ack == DAP_TRANSFER_OK) {
      /* 等待2ms让目标处理 */
      dap_os_delay(2);
      /* 构建AP写请求：选择AP，地址位A2和A3（DRW寄存器） */
      req = DAP_TRANSFER_APnDP | 0 | DAP_TRANSFER_A2 | DAP_TRANSFER_A3;
      /* 写入复位值到DRW寄存器触发复位 */
      SWD_Transfer(req, &AIRCR_RESET_VAL);
    }
  }
#endif

  /* 执行硬件复位并存储结果 */
  *(response+1) = RESET_TARGET();
  /* 设置操作状态为成功 */
  *(response+0) = DAP_OK;
  return (2U);
}


/*
 * 处理SWJ引脚控制命令并准备响应
 * 
 * 参数:
 *   request:  指向请求数据的指针
 *   response: 指向响应数据的指针
 * 
 * 返回值:
 *   响应数据的字节数（低16位）
 *   请求数据的字节数（高16位）
 * 
 * 功能：控制SWJ调试接口的各个引脚状态
 * 
 * 请求数据格式:
 *   byte 0: 引脚输出值（位掩码）
 *   byte 1: 引脚选择掩码（选择要控制的引脚）
 *   byte 2-5: 等待时间（微秒，用于等待引脚状态稳定）
 * 
 * 引脚位定义:
 *   bit 0: SWCLK/TCK
 *   bit 1: SWDIO/TMS
 *   bit 2: TDI
 *   bit 3: TDO（仅输入）
 *   bit 5: nTRST
 *   bit 7: nRESET
 */
static uint32_t DAP_SWJ_Pins(const uint8_t *request, uint8_t *response) {
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
  /* 如果当前使用SPI传输模式，需要先反初始化以释放GPIO控制权 */
  if (SWD_TransferSpeed == kTransfer_SPI)
    DAP_SPI_Deinit();

  uint32_t value;      /* 引脚输出值 */
  uint32_t select;     /* 引脚选择掩码 */
  uint32_t wait;       /* 等待时间（微秒） */
  uint32_t timestamp;  /* 时间戳 */

  /* 解析请求数据 */
  value  = (uint32_t) *(request+0);
  select = (uint32_t) *(request+1);
  /* 组合4字节等待时间（小端序） */
  wait   = (uint32_t)(*(request+2) <<  0) |
           (uint32_t)(*(request+3) <<  8) |
           (uint32_t)(*(request+4) << 16) |
           (uint32_t)(*(request+5) << 24);

  /* 控制SWCLK/TCK引脚 */
  if ((select & (1U << DAP_SWJ_SWCLK_TCK)) != 0U) {
    if ((value & (1U << DAP_SWJ_SWCLK_TCK)) != 0U) {
      PIN_SWCLK_TCK_SET();  /* 设置为高电平 */
    } else {
      PIN_SWCLK_TCK_CLR();  /* 设置为低电平 */
    }
  }
  /* 控制SWDIO/TMS引脚 */
  if ((select & (1U << DAP_SWJ_SWDIO_TMS)) != 0U) {
    if ((value & (1U << DAP_SWJ_SWDIO_TMS)) != 0U) {
      PIN_SWDIO_TMS_SET();  /* 设置为高电平 */
    } else {
      PIN_SWDIO_TMS_CLR();  /* 设置为低电平 */
    }
  }
  /* 控制TDI引脚 */
  if ((select & (1U << DAP_SWJ_TDI)) != 0U) {
    PIN_TDI_OUT(value >> DAP_SWJ_TDI);
  }
  /* 控制nTRST引脚（JTAG测试复位） */
  if ((select & (1U << DAP_SWJ_nTRST)) != 0U) {
    PIN_nTRST_OUT(value >> DAP_SWJ_nTRST);
  }
  /* 控制nRESET引脚（目标复位） */
  if ((select & (1U << DAP_SWJ_nRESET)) != 0U){
    PIN_nRESET_OUT(value >> DAP_SWJ_nRESET);
  }

  /* 如果指定了等待时间，则等待引脚状态稳定 */
  if (wait != 0U) {
#if (TIMESTAMP_CLOCK != 0U)
    /* 限制最大等待时间为3秒 */
    if (wait > 3000000U) {
      wait = 3000000U;
    }
    /* 将微秒转换为时间戳时钟周期数 */
#if (TIMESTAMP_CLOCK >= 1000000U)
    wait *= TIMESTAMP_CLOCK / 1000000U;
#else
    wait /= 1000000U / TIMESTAMP_CLOCK;
#endif
#else
    wait  = 1U;  /* 如果没有时间戳时钟，只等待一个周期 */
#endif
    /* 获取当前时间戳 */
    timestamp = TIMESTAMP_GET();
    /* 轮询等待引脚状态匹配期望值 */
    do {
      /* 检查SWCLK/TCK引脚状态 */
      if ((select & (1U << DAP_SWJ_SWCLK_TCK)) != 0U) {
        if ((value >> DAP_SWJ_SWCLK_TCK) ^ PIN_SWCLK_TCK_IN()) {
          continue;  /* 状态不匹配，继续等待 */
        }
      }
      /* 检查SWDIO/TMS引脚状态 */
      if ((select & (1U << DAP_SWJ_SWDIO_TMS)) != 0U) {
        if ((value >> DAP_SWJ_SWDIO_TMS) ^ PIN_SWDIO_TMS_IN()) {
          continue;
        }
      }
      /* 检查TDI引脚状态 */
      if ((select & (1U << DAP_SWJ_TDI)) != 0U) {
        if ((value >> DAP_SWJ_TDI) ^ PIN_TDI_IN()) {
          continue;
        }
      }
      /* 检查nTRST引脚状态 */
      if ((select & (1U << DAP_SWJ_nTRST)) != 0U) {
        if ((value >> DAP_SWJ_nTRST) ^ PIN_nTRST_IN()) {
          continue;
        }
      }
      /* 检查nRESET引脚状态 */
      if ((select & (1U << DAP_SWJ_nRESET)) != 0U) {
        if ((value >> DAP_SWJ_nRESET) ^ PIN_nRESET_IN()) {
          continue;
        }
      }
      break;  /* 所有选中的引脚状态都匹配，退出循环 */
    } while ((TIMESTAMP_GET() - timestamp) < wait);
  }

  /* 读取所有引脚的当前状态并组合成响应值 */
  value = (PIN_SWCLK_TCK_IN() << DAP_SWJ_SWCLK_TCK) |
          (PIN_SWDIO_TMS_IN() << DAP_SWJ_SWDIO_TMS) |
          (PIN_TDI_IN()       << DAP_SWJ_TDI)       |
          (PIN_TDO_IN()       << DAP_SWJ_TDO)       |
          (PIN_nTRST_IN()     << DAP_SWJ_nTRST)     |
          (PIN_nRESET_IN()    << DAP_SWJ_nRESET);

  *response = (uint8_t)value;
#else
  /* SWD和JTAG都未启用时返回0 */
  *response = 0U;
#endif

  /* 如果之前使用SPI模式，恢复SPI初始化 */
  if (SWD_TransferSpeed == kTransfer_SPI)
    DAP_SPI_Init();

  /* 返回：请求6字节，响应1字节 */
  return ((6U << 16) | 1U);
}

/* SWD传输速度模式变量声明 */
extern uint8_t SWD_TransferSpeed;

/**
 * @brief 处理SWJ时钟命令并准备响应
 * 
 * 该函数根据请求的时钟频率配置DAP的时钟参数，
 * 选择合适的传输模式（SPI或GPIO）以达到最佳性能。
 * 
 * @param request  指向请求数据的指针，包含4字节的时钟频率值（小端序）
 * @param response 指向响应数据的指针，返回操作状态
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数
 *                  - 高16位：请求数据的字节数
 */
static uint32_t DAP_SWJ_Clock(const uint8_t *request, uint8_t *response) {
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
  uint32_t clock;  /* 目标时钟频率（Hz） */
  uint32_t delay;  /* 计算得到的延迟周期数 */

  /* 从请求数据中提取4字节的时钟频率值（小端序） */
  clock = (uint32_t)(*(request+0) <<  0) |
          (uint32_t)(*(request+1) <<  8) |
          (uint32_t)(*(request+2) << 16) |
          (uint32_t)(*(request+3) << 24);

  /* 时钟频率为0是无效值，返回错误 */
  if (clock == 0U) {
    *response = DAP_ERROR;
    return ((4U << 16) | 1U);
  }

  /* 
   * 注意：ESP8266的最大IO频率低于2MHz
   * 根据不同的时钟频率选择不同的传输模式
   */

  /* 
   * 高速模式：时钟频率 >= 10MHz
   * 使用40MHz SPI进行传输（仅SWD模式支持）
   */
  if (clock >= 10000000) {
    if (DAP_Data.debug_port != DAP_PORT_JTAG) {
      /* SWD模式：初始化SPI并使用SPI传输 */
      DAP_SPI_Init();
      SWD_TransferSpeed = kTransfer_SPI;
    } else {
      /* JTAG模式：不支持SPI，使用快速GPIO模式 */
      SWD_TransferSpeed = kTransfer_GPIO_fast;
    }
    DAP_Data.fast_clock  = 1U;  /* 启用快速时钟模式 */
    DAP_Data.clock_delay = 1U;  /* 最小延迟 */

  } else if (clock >= 2000000) {
    /* 
     * 中速模式：时钟频率 >= 2MHz 且 < 10MHz
     * 使用GPIO快速模式，无程序延迟
     */
    DAP_SPI_Deinit();  /* 关闭SPI，切换到GPIO模式 */
    DAP_Data.fast_clock  = 1U;  /* 启用快速时钟模式 */
    DAP_Data.clock_delay = 1U;  /* 最小延迟 */
    SWD_TransferSpeed = kTransfer_GPIO_fast;
  } else {
    /* 
     * 低速模式：时钟频率 < 2MHz
     * 使用GPIO普通模式，带程序延迟以降低频率
     */
    DAP_SPI_Deinit();  /* 关闭SPI，切换到GPIO模式 */
    DAP_Data.fast_clock  = 0U;  /* 禁用快速时钟模式 */
    SWD_TransferSpeed = kTransfer_GPIO_normal;

    /* 
     * 根据不同的ESP芯片定义总线时钟频率
     * 用于计算精确的延迟周期
     */
#ifdef CONFIG_IDF_TARGET_ESP8266
  #define BUS_CLOCK_FIXED 80000000   /* ESP8266: 80MHz */
#elif defined CONFIG_IDF_TARGET_ESP32 || defined CONFIG_IDF_TARGET_ESP32S3
  #define BUS_CLOCK_FIXED 100000000  /* ESP32/ESP32S3: 100MHz */
#elif defined CONFIG_IDF_TARGET_ESP32C3
  #define BUS_CLOCK_FIXED 80000000   /* ESP32C3: 80MHz */
#endif

    /* 
     * 计算延迟周期数
     * 公式：delay = (总线时钟/2 + 目标时钟 - 1) / 目标时钟
     * 这样可以得到向上取整的结果
     */
    delay = ((BUS_CLOCK_FIXED/2U) + (clock - 1U)) / clock;
    
    /* 
     * 减去IO端口写入周期的固有延迟
     * 然后转换为慢速延迟循环次数
     */
    if (delay > IO_PORT_WRITE_CYCLES) {
      delay -= IO_PORT_WRITE_CYCLES;
      /* 向上取整到DELAY_SLOW_CYCLES的倍数 */
      delay  = (delay + (DELAY_SLOW_CYCLES - 1U)) / DELAY_SLOW_CYCLES;
    } else {
      /* 延迟太小，使用最小值1 */
      delay  = 1U;
    }

    DAP_Data.clock_delay = delay;  /* 保存计算得到的延迟值 */
  }

  *response = DAP_OK;  /* 操作成功 */
#else
  /* SWD和JTAG都未启用时返回错误 */
  *response = DAP_ERROR;
#endif

  /* 返回：请求4字节，响应1字节 */
  return ((4U << 16) | 1U);
}


/**
 * @brief 处理SWJ序列命令并准备响应
 * 
 * 该函数用于发送SWJ（SWD/JTAG共用）序列，通常用于：
 * - SWD到JTAG的切换序列
 * - JTAG到SWD的切换序列
 * - 线路复位序列
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: 序列位数（0表示256位）
 *                 - byte 1+: 序列数据（LSB优先）
 * @param response 指向响应数据的指针，返回操作状态
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数
 *                  - 高16位：请求数据的字节数
 */
static uint32_t DAP_SWJ_Sequence(const uint8_t *request, uint8_t *response) {
  uint32_t count;  /* 序列位数 */

  /* 获取序列位数，0表示256位 */
  count = *request++;
  if (count == 0U) {
    count = 256U;
  }

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
  /* 发送SWJ序列 */
  SWJ_Sequence(count, request);
  *response = DAP_OK;  /* 操作成功 */
#else
  /* SWD和JTAG都未启用时返回错误 */
  *response = DAP_ERROR;
#endif

  /* 将位数转换为字节数（向上取整） */
  count = (count + 7U) >> 3;

  /* 返回：请求(count+1)字节，响应1字节 */
  return (((count + 1U) << 16) | 1U);
}


/**
 * @brief 处理SWD配置命令并准备响应
 * 
 * 该函数配置SWD传输的参数，包括：
 * - 转换周期数：数据方向切换时的空闲时钟周期
 * - 数据阶段：是否在WAIT/FAULT响应后继续数据阶段
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: 配置值
 *                   - bit [1:0]: 转换周期数-1（0=1周期，1=2周期，2=3周期，3=4周期）
 *                   - bit 2: 数据阶段（0=禁用，1=启用）
 * @param response 指向响应数据的指针，返回操作状态
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数
 *                  - 高16位：请求数据的字节数
 */
static uint32_t DAP_SWD_Configure(const uint8_t *request, uint8_t *response) {
#if (DAP_SWD != 0)
  uint8_t value;  /* 配置值 */

  value = *request;
  /* 提取转换周期数（bit[1:0] + 1，范围1-4） */
  DAP_Data.swd_conf.turnaround = (value & 0x03U) + 1U;
  /* 提取数据阶段配置（bit 2） */
  DAP_Data.swd_conf.data_phase = (value & 0x04U) ? 1U : 0U;

  *response = DAP_OK;  /* 操作成功 */
#else
  /* SWD未启用时返回错误 */
  *response = DAP_ERROR;
#endif

  /* 返回：请求1字节，响应1字节 */
  return ((1U << 16) | 1U);
}


/**
 * @brief 处理SWD序列命令并准备响应
 * 
 * 该函数执行一系列SWD时钟序列操作，支持：
 * - 多个连续的序列操作
 * - 每个序列可以是输入或输出方向
 * - 灵活的时钟周期数（1-64）
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: 序列数量
 *                 - 对于每个序列:
 *                   - byte: 序列信息
 *                     - bit [5:0]: 时钟周期数（0=64周期）
 *                     - bit 7: 方向（0=输出，1=输入）
 *                   - 输出序列后跟数据字节
 * @param response 指向响应数据的指针
 *                 - byte 0: 操作状态
 *                 - 输入序列的读取数据
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数
 *                  - 高16位：请求数据的字节数
 */
static uint32_t DAP_SWD_Sequence(const uint8_t *request, uint8_t *response) {
  uint32_t sequence_info;   /* 当前序列的信息字节 */
  uint32_t sequence_count;  /* 剩余序列数量 */
  uint32_t request_count;   /* 请求数据总字节数 */
  uint32_t response_count;  /* 响应数据总字节数 */
  uint32_t count;           /* 当前序列的数据字节数 */

#if (DAP_SWD != 0)
  *response++ = DAP_OK;  /* 操作成功 */
#else
  *response++ = DAP_ERROR;  /* SWD未启用时返回错误 */
#endif
  request_count  = 1U;  /* 初始计数：序列数量字节 */
  response_count = 1U;  /* 初始计数：状态字节 */

  /* 获取序列数量 */
  sequence_count = *request++;
  
  /* 处理每个序列 */
  while (sequence_count--) {
    /* 获取序列信息字节 */
    sequence_info = *request++;
    
    /* 提取时钟周期数（bit[5:0]，0表示64） */
    count = sequence_info & SWD_SEQUENCE_CLK;
    if (count == 0U) {
      count = 64U;
    }
    /* 将位数转换为字节数（向上取整） */
    count = (count + 7U) / 8U;
    
#if (DAP_SWD != 0)
    /* 
     * 注意：以下代码被注释掉，可能是因为SWD_Sequence内部已处理方向切换
     * 如果需要手动控制SWDIO方向，可以取消注释
     */
    // if ((sequence_info & SWD_SEQUENCE_DIN) != 0U) {
    //   PIN_SWDIO_OUT_DISABLE();  /* 输入模式：禁用输出 */
    // } else {
    //   PIN_SWDIO_OUT_ENABLE();   /* 输出模式：启用输出 */
    // }
    
    /* 执行SWD序列操作 */
    SWD_Sequence(sequence_info, request, response);
    
    // if (sequence_count == 0U) {
    //   PIN_SWDIO_OUT_ENABLE();  /* 最后一个序列后恢复输出模式 */
    // }
#endif

    /* 根据序列方向更新计数器 */
    if ((sequence_info & SWD_SEQUENCE_DIN) != 0U) {
      /* 输入序列：数据从目标读取到响应 */
      request_count++;  /* 只有序列信息字节 */
#if (DAP_SWD != 0)
      response += count;       /* 响应指针前移 */
      response_count += count; /* 累加响应字节数 */
#endif
    } else {
      /* 输出序列：数据从请求写入到目标 */
      request += count;            /* 请求指针跳过数据 */
      request_count += count + 1U; /* 累加序列信息和数据字节数 */
    }
  }

  /* 返回：请求request_count字节，响应response_count字节 */
  return ((request_count << 16) | response_count);
}


/**
 * @brief 处理JTAG序列命令并准备响应
 * 
 * 该函数执行一系列JTAG时钟序列操作，支持：
 * - 多个连续的序列操作
 * - 每个序列可配置TCK时钟周期数
 * - 可选的TDO数据捕获
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: 序列数量
 *                 - 对于每个序列:
 *                   - byte: 序列信息
 *                     - bit [5:0]: TCK时钟周期数（0=64周期）
 *                     - bit 7: TDO捕获标志（1=捕获TDO数据）
 *                   - 后跟TDI数据字节
 * @param response 指向响应数据的指针
 *                 - byte 0: 操作状态
 *                 - TDO捕获的数据（如果请求）
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数
 *                  - 高16位：请求数据的字节数
 */
static uint32_t DAP_JTAG_Sequence(const uint8_t *request, uint8_t *response) {
  uint32_t sequence_info;   /* 当前序列的信息字节 */
  uint32_t sequence_count;  /* 剩余序列数量 */
  uint32_t request_count;   /* 请求数据总字节数 */
  uint32_t response_count;  /* 响应数据总字节数 */
  uint32_t count;           /* 当前序列的数据字节数 */

#if (DAP_JTAG != 0)
  *response++ = DAP_OK;     /* JTAG启用时返回成功 */
#else
  *response++ = DAP_ERROR;  /* JTAG未启用时返回错误 */
#endif
  request_count  = 1U;      /* 初始计数：序列数量字节 */
  response_count = 1U;      /* 初始计数：状态字节 */

  /* 获取序列数量 */
  sequence_count = *request++;
  
  /* 处理每个序列 */
  while (sequence_count--) {
    /* 获取序列信息字节 */
    sequence_info = *request++;
    
    /* 提取TCK时钟周期数（bit[5:0]，0表示64） */
    count = sequence_info & JTAG_SEQUENCE_TCK;
    if (count == 0U) {
      count = 64U;
    }
    
    /* 将位数转换为字节数（向上取整） */
    count = (count + 7U) / 8U;
    
#if (DAP_JTAG != 0)
    /* 执行JTAG序列操作 */
    JTAG_Sequence(sequence_info, request, response);
#endif
    
    /* 请求指针跳过TDI数据 */
    request += count;
    /* 累加序列信息字节和数据字节 */
    request_count += count + 1U;
    
#if (DAP_JTAG != 0)
    /* 如果请求TDO捕获，更新响应计数 */
    if ((sequence_info & JTAG_SEQUENCE_TDO) != 0U) {
      response += count;       /* 响应指针前移 */
      response_count += count; /* 累加响应字节数 */
    }
#endif
  }

  /* 返回：请求request_count字节，响应response_count字节 */
  return ((request_count << 16) | response_count);
}


/**
 * @brief 处理JTAG配置命令并准备响应
 * 
 * 该函数配置JTAG扫描链中各TAP的IR长度信息，用于：
 * - 设置扫描链中TAP设备的数量
 * - 记录每个TAP的IR寄存器长度
 * - 计算每个TAP的IR前后位数（用于bypass）
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: TAP设备数量
 *                 - byte 1..n: 每个TAP的IR长度
 * @param response 指向响应数据的指针
 *                 - byte 0: 操作状态
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数（1）
 *                  - 高16位：请求数据的字节数（count+1）
 */
static uint32_t DAP_JTAG_Configure(const uint8_t *request, uint8_t *response) {
  uint32_t count;           /* TAP设备数量 */
#if (DAP_JTAG != 0)
  uint32_t length;          /* 当前TAP的IR长度 */
  uint32_t bits;            /* IR位数累加器 */
  uint32_t n;               /* 循环计数器 */

  /* 获取TAP设备数量 */
  count = *request++;
  DAP_Data.jtag_dev.count = (uint8_t)count;

  /* 第一遍：计算每个TAP之前的IR位数 */
  bits = 0U;
  for (n = 0U; n < count; n++) {
    length = *request++;
    DAP_Data.jtag_dev.ir_length[n] =  (uint8_t)length;   /* 存储IR长度 */
    DAP_Data.jtag_dev.ir_before[n] = (uint16_t)bits;     /* 此TAP之前的总IR位数 */
    bits += length;                                       /* 累加IR位数 */
  }
  
  /* 第二遍：计算每个TAP之后的IR位数 */
  for (n = 0U; n < count; n++) {
    bits -= DAP_Data.jtag_dev.ir_length[n];              /* 减去当前TAP的IR长度 */
    DAP_Data.jtag_dev.ir_after[n] = (uint16_t)bits;      /* 此TAP之后的总IR位数 */
  }

  *response = DAP_OK;       /* 配置成功 */
#else
  count = *request;         /* 仅读取count用于返回值计算 */
  *response = DAP_ERROR;    /* JTAG未启用时返回错误 */
#endif

  /* 返回：请求(count+1)字节，响应1字节 */
  return (((count + 1U) << 16) | 1U);
}


/**
 * @brief 处理JTAG IDCODE命令并准备响应
 * 
 * 该函数读取指定TAP设备的IDCODE寄存器，用于：
 * - 识别JTAG扫描链中的设备
 * - 验证设备连接和配置
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: TAP设备索引
 * @param response 指向响应数据的指针
 *                 - byte 0: 操作状态
 *                 - byte 1-4: IDCODE值（小端序，仅成功时）
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数（成功5，失败1）
 *                  - 高16位：请求数据的字节数（1）
 */
static uint32_t DAP_JTAG_IDCode(const uint8_t *request, uint8_t *response) {
#if (DAP_JTAG != 0)
  uint32_t data;            /* IDCODE寄存器值 */

  /* 验证当前调试端口是否为JTAG */
  if (DAP_Data.debug_port != DAP_PORT_JTAG) {
    goto id_error;
  }

  /* 获取并验证TAP设备索引 */
  DAP_Data.jtag_dev.index = *request;
  if (DAP_Data.jtag_dev.index >= DAP_Data.jtag_dev.count) {
    goto id_error;          /* 索引超出范围 */
  }

  /* 选择JTAG扫描链，加载IDCODE指令 */
  JTAG_IR(JTAG_IDCODE);

  /* 读取IDCODE寄存器 */
  data = JTAG_ReadIDCode();

  /* 存储结果数据（小端序） */
  *(response+0) =  DAP_OK;                    /* 操作成功 */
  *(response+1) = (uint8_t)(data >>  0);      /* IDCODE byte 0 (LSB) */
  *(response+2) = (uint8_t)(data >>  8);      /* IDCODE byte 1 */
  *(response+3) = (uint8_t)(data >> 16);      /* IDCODE byte 2 */
  *(response+4) = (uint8_t)(data >> 24);      /* IDCODE byte 3 (MSB) */

  /* 返回：请求1字节，响应5字节 */
  return ((1U << 16) | 5U);

id_error:
#endif
  /* 错误处理：返回错误状态 */
  *response = DAP_ERROR;
  /* 返回：请求1字节，响应1字节 */
  return ((1U << 16) | 1U);
}


/**
 * @brief 处理传输配置命令并准备响应
 * 
 * 该函数配置SWD/JTAG传输参数，包括：
 * - 空闲周期数：传输后的额外时钟周期
 * - 重试次数：WAIT响应时的重试次数
 * - 匹配重试次数：值匹配操作的重试次数
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: 空闲周期数
 *                 - byte 1-2: 重试次数（16位小端序）
 *                 - byte 3-4: 匹配重试次数（16位小端序）
 * @param response 指向响应数据的指针
 *                 - byte 0: 操作状态
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数（1）
 *                  - 高16位：请求数据的字节数（5）
 */
static uint32_t DAP_TransferConfigure(const uint8_t *request, uint8_t *response) {

  /* 配置空闲周期数（传输后的额外SWCLK/TCK周期） */
  DAP_Data.transfer.idle_cycles =            *(request+0);
  
  /* 配置WAIT响应重试次数（16位小端序） */
  DAP_Data.transfer.retry_count = (uint16_t) *(request+1) |
                                  (uint16_t)(*(request+2) << 8);
  
  /* 配置值匹配重试次数（16位小端序） */
  DAP_Data.transfer.match_retry = (uint16_t) *(request+3) |
                                  (uint16_t)(*(request+4) << 8);

  *response = DAP_OK;       /* 配置成功 */
  
  /* 返回：请求5字节，响应1字节 */
  return ((5U << 16) | 1U);
}

/**
 * @brief 处理SWD传输命令并准备响应
 * 
 * 该函数执行SWD协议的数据传输操作，支持以下功能：
 * - DP（调试端口）和AP（访问端口）寄存器的读写操作
 * - 值匹配读取：持续读取直到数据匹配指定值
 * - 时间戳记录：可选择性地记录传输时间戳
 * - 写入验证：通过读取RDBUFF验证写操作是否成功
 * 
 * @param request  指向请求数据的指针
 *                 - byte 0: DAP索引（被忽略）
 *                 - byte 1: 传输请求计数
 *                 - byte 2+: 传输请求序列
 *                   每个请求包含：
 *                   - 1字节请求值（包含RnW、APnDP、地址等标志）
 *                   - 写操作时：4字节数据
 *                   - 值匹配读取时：4字节匹配值
 * @param response 指向响应数据的指针
 *                 - byte 0: 已完成的传输计数
 *                 - byte 1: 最后一次传输的响应值
 *                 - byte 2+: 读取的数据和可选的时间戳
 * @return uint32_t 返回值包含：
 *                  - 低16位：响应数据的字节数
 *                  - 高16位：请求数据的字节数
 */
#if (DAP_SWD != 0)
static uint32_t DAP_SWD_Transfer(const uint8_t *request, uint8_t *response) {
  const
  uint8_t  *request_head;       /* 请求数据起始指针，用于计算处理的字节数 */
  uint32_t  request_count;      /* 待处理的传输请求计数 */
  uint32_t  request_value;      /* 当前传输请求的值（包含操作类型和地址） */
  uint8_t  *response_head;      /* 响应数据起始指针 */
  uint32_t  response_count;     /* 已完成的传输计数 */
  uint32_t  response_value;     /* 最后一次传输的响应状态 */
  uint32_t  post_read;          /* AP读取延迟标志：1表示有待处理的AP读取数据 */
  uint32_t  check_write;        /* 写入验证标志：1表示需要验证最后的写操作 */
  uint32_t  match_value;        /* 值匹配操作的目标值 */
  uint32_t  match_retry;        /* 值匹配操作的剩余重试次数 */
  uint32_t  retry;              /* WAIT响应的剩余重试次数 */
  uint32_t  data;               /* 传输的数据 */
#if (TIMESTAMP_CLOCK != 0U)
  uint32_t  timestamp;          /* 时间戳值 */
#endif

  /* 保存请求起始位置 */
  request_head   = request;

  /* 初始化响应计数器和状态 */
  response_count = 0U;
  response_value = 0U;
  response_head  = response;
  response      += 2;           /* 跳过响应头的2字节（计数和状态） */

  /* 清除传输中止标志 */
  DAP_TransferAbort = 0U;

  /* 初始化延迟读取和写入验证标志 */
  post_read   = 0U;
  check_write = 0U;

  request++;                    /* 跳过DAP索引字节 */

  /* 获取传输请求计数 */
  request_count = *request++;

  /* 处理每个传输请求 */
  for (; request_count != 0U; request_count--) {
    /* 获取当前请求值 */
    request_value = *request++;
    
    if ((request_value & DAP_TRANSFER_RnW) != 0U) {
      /* ========== 读取寄存器操作 ========== */
      
      if (post_read) {
        /* 
         * 之前有AP读取被延迟（post_read），需要先获取该数据
         * AP读取是流水线式的：发起读取后，数据在下一次传输时返回
         */
        retry = DAP_Data.transfer.retry_count;
        
        if ((request_value & (DAP_TRANSFER_APnDP | DAP_TRANSFER_MATCH_VALUE)) == DAP_TRANSFER_APnDP) {
          /* 
           * 当前也是普通AP读取：
           * 读取上一次AP数据的同时，发起新的AP读取请求
           */
          do {
            response_value = SWD_Transfer(request_value, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        } else {
          /* 
           * 当前不是普通AP读取（可能是DP读取或值匹配读取）：
           * 通过读取RDBUFF获取上一次AP数据
           */
          do {
            response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          post_read = 0U;       /* 清除延迟读取标志 */
        }
        
        if (response_value != DAP_TRANSFER_OK) {
          break;                /* 传输失败，退出循环 */
        }
        
        /* 将上一次AP读取的数据存入响应缓冲区（小端序） */
        *response++ = (uint8_t) data;
        *response++ = (uint8_t)(data >>  8);
        *response++ = (uint8_t)(data >> 16);
        *response++ = (uint8_t)(data >> 24);
        
#if (TIMESTAMP_CLOCK != 0U)
        if (post_read) {
          /* 如果同时发起了新的AP读取，存储时间戳 */
          if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
            timestamp = DAP_Data.timestamp;
            *response++ = (uint8_t) timestamp;
            *response++ = (uint8_t)(timestamp >>  8);
            *response++ = (uint8_t)(timestamp >> 16);
            *response++ = (uint8_t)(timestamp >> 24);
          }
        }
#endif
      }
      
      if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
        /* 
         * ========== 值匹配读取 ==========
         * 持续读取寄存器直到值与指定值匹配，或重试次数耗尽
         */
        
        /* 从请求中提取匹配值（小端序） */
        match_value = (uint32_t)(*(request+0) <<  0) |
                      (uint32_t)(*(request+1) <<  8) |
                      (uint32_t)(*(request+2) << 16) |
                      (uint32_t)(*(request+3) << 24);
        request += 4;
        
        /* 获取匹配重试次数 */
        match_retry = DAP_Data.transfer.match_retry;
        
        if ((request_value & DAP_TRANSFER_APnDP) != 0U) {
          /* AP寄存器：先发起AP读取请求（数据在下次传输返回） */
          retry = DAP_Data.transfer.retry_count;
          do {
            response_value = SWD_Transfer(request_value, NULL);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if (response_value != DAP_TRANSFER_OK) {
            break;
          }
        }
        
        /* 循环读取直到值匹配或重试耗尽 */
        do {
          retry = DAP_Data.transfer.retry_count;
          do {
            response_value = SWD_Transfer(request_value, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if (response_value != DAP_TRANSFER_OK) {
            break;
          }
        } while (((data & DAP_Data.transfer.match_mask) != match_value) && match_retry-- && !DAP_TransferAbort);
        
        /* 检查是否匹配成功 */
        if ((data & DAP_Data.transfer.match_mask) != match_value) {
          response_value |= DAP_TRANSFER_MISMATCH;  /* 设置不匹配标志 */
        }
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
      } else {
        /* 
         * ========== 普通读取 ==========
         */
        retry = DAP_Data.transfer.retry_count;
        
        if ((request_value & DAP_TRANSFER_APnDP) != 0U) {
          /* 
           * AP寄存器读取：
           * AP读取是流水线式的，发起读取后数据在下次传输返回
           */
          if (post_read == 0U) {
            /* 发起AP读取请求 */
            do {
              response_value = SWD_Transfer(request_value, NULL);
            } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK) {
              break;
            }
#if (TIMESTAMP_CLOCK != 0U)
            /* 存储时间戳 */
            if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
              timestamp = DAP_Data.timestamp;
              *response++ = (uint8_t) timestamp;
              *response++ = (uint8_t)(timestamp >>  8);
              *response++ = (uint8_t)(timestamp >> 16);
              *response++ = (uint8_t)(timestamp >> 24);
            }
#endif
            post_read = 1U;     /* 设置延迟读取标志 */
          }
        } else {
          /* 
           * DP寄存器读取：
           * DP读取是即时的，数据立即返回
           */
          do {
            response_value = SWD_Transfer(request_value, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if (response_value != DAP_TRANSFER_OK) {
            break;
          }
#if (TIMESTAMP_CLOCK != 0U)
          /* 存储时间戳 */
          if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
            timestamp = DAP_Data.timestamp;
            *response++ = (uint8_t) timestamp;
            *response++ = (uint8_t)(timestamp >>  8);
            *response++ = (uint8_t)(timestamp >> 16);
            *response++ = (uint8_t)(timestamp >> 24);
          }
#endif
          /* 存储读取的数据（小端序） */
          *response++ = (uint8_t) data;
          *response++ = (uint8_t)(data >>  8);
          *response++ = (uint8_t)(data >> 16);
          *response++ = (uint8_t)(data >> 24);
        }
      }
      check_write = 0U;         /* 清除写入验证标志 */
    } else {
      /* ========== 写入寄存器操作 ========== */
      
      if (post_read) {
        /* 
         * 之前有AP读取被延迟，需要先获取该数据
         * 通过读取RDBUFF获取数据
         */
        retry = DAP_Data.transfer.retry_count;
        do {
          response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
        } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
        /* 存储之前的AP读取数据 */
        *response++ = (uint8_t) data;
        *response++ = (uint8_t)(data >>  8);
        *response++ = (uint8_t)(data >> 16);
        *response++ = (uint8_t)(data >> 24);
        post_read = 0U;
      }
      
      /* 从请求中提取要写入的数据（小端序） */
      data = (uint32_t)(*(request+0) <<  0) |
             (uint32_t)(*(request+1) <<  8) |
             (uint32_t)(*(request+2) << 16) |
             (uint32_t)(*(request+3) << 24);
      request += 4;
      
      if ((request_value & DAP_TRANSFER_MATCH_MASK) != 0U) {
        /* 
         * 写入匹配掩码：
         * 这不是真正的寄存器写入，而是设置值匹配操作的掩码
         * 该掩码用于后续的值匹配读取操作，决定哪些位参与比较
         */
        DAP_Data.transfer.match_mask = data;
        response_value = DAP_TRANSFER_OK;
      } else {
        /* 
         * 写入DP/AP寄存器：
         * 执行实际的SWD写入传输
         */
        retry = DAP_Data.transfer.retry_count;
        do {
          response_value = SWD_Transfer(request_value, &data);
        } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
#if (TIMESTAMP_CLOCK != 0U)
        /* 存储时间戳（如果请求中设置了时间戳标志） */
        if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
          timestamp = DAP_Data.timestamp;
          *response++ = (uint8_t) timestamp;
          *response++ = (uint8_t)(timestamp >>  8);
          *response++ = (uint8_t)(timestamp >> 16);
          *response++ = (uint8_t)(timestamp >> 24);
        }
#endif
        check_write = 1U;       /* 设置写入验证标志，用于循环结束后验证写操作 */
      }
    }
    
    response_count++;           /* 增加已完成传输计数 */
    
    /* 检查是否收到传输中止信号 */
    if (DAP_TransferAbort) {
      break;
    }
  }

  /* 
   * ========== 处理被取消的请求 ==========
   * 如果传输被中止，需要跳过剩余的请求数据以正确计算请求长度
   */
  for (; request_count != 0U; request_count--) {
    request_value = *request++;
    if ((request_value & DAP_TRANSFER_RnW) != 0U) {
      /* 读取请求 */
      if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
        /* 值匹配读取：跳过4字节的匹配值 */
        request += 4;
      }
    } else {
      /* 写入请求：跳过4字节的写入数据 */
      request += 4;
    }
  }

  /* 
   * ========== 传输完成后的收尾处理 ==========
   */
  if (response_value == DAP_TRANSFER_OK) {
    if (post_read) {
      /* 
       * 获取最后一次AP读取的数据：
       * 由于AP读取是流水线式的，最后一次AP读取的数据
       * 需要通过读取RDBUFF来获取
       */
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
      /* 存储最后的AP读取数据（小端序） */
      *response++ = (uint8_t) data;
      *response++ = (uint8_t)(data >>  8);
      *response++ = (uint8_t)(data >> 16);
      *response++ = (uint8_t)(data >> 24);
    } else if (check_write) {
      /* 
       * 验证最后的写操作：
       * 通过读取RDBUFF检查写操作是否成功完成
       * 如果写操作产生了错误，RDBUFF读取会返回错误状态
       */
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
    }
  }

end:
  /* 
   * ========== 填充响应头 ==========
   * response_head[0]: 已完成的传输计数
   * response_head[1]: 最后一次传输的响应状态
   */
  *(response_head+0) = (uint8_t)response_count;
  *(response_head+1) = (uint8_t)response_value;

  /* 
   * 返回值：
   * - 高16位：已处理的请求字节数
   * - 低16位：响应数据的字节数
   */
  return (((uint32_t)(request - request_head) << 16) | (uint32_t)(response - response_head));
}
#endif


// Process JTAG Transfer command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
#if (DAP_JTAG != 0)
static uint32_t DAP_JTAG_Transfer(const uint8_t *request, uint8_t *response) {
  const
  uint8_t  *request_head;
  uint32_t  request_count;
  uint32_t  request_value;
  uint32_t  request_ir;
  uint8_t  *response_head;
  uint32_t  response_count;
  uint32_t  response_value;
  uint32_t  post_read;
  uint32_t  match_value;
  uint32_t  match_retry;
  uint32_t  retry;
  uint32_t  data;
  uint32_t  ir;
#if (TIMESTAMP_CLOCK != 0U)
  uint32_t  timestamp;
#endif

  request_head   = request;

  response_count = 0U;
  response_value = 0U;
  response_head  = response;
  response      += 2;

  DAP_TransferAbort = 0U;

  ir        = 0U;
  post_read = 0U;

  // Device index (JTAP TAP)
  DAP_Data.jtag_dev.index = *request++;
  if (DAP_Data.jtag_dev.index >= DAP_Data.jtag_dev.count) {
    goto end;
  }

  request_count = *request++;

  for (; request_count != 0U; request_count--) {
    request_value = *request++;
    request_ir = (request_value & DAP_TRANSFER_APnDP) ? JTAG_APACC : JTAG_DPACC;
    if ((request_value & DAP_TRANSFER_RnW) != 0U) {
      // Read register
      if (post_read) {
        // Read was posted before
        retry = DAP_Data.transfer.retry_count;
        if ((ir == request_ir) && ((request_value & DAP_TRANSFER_MATCH_VALUE) == 0U)) {
          // Read previous data and post next read
          do {
            response_value = JTAG_Transfer(request_value, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        } else {
          // Select JTAG chain
          if (ir != JTAG_DPACC) {
            ir = JTAG_DPACC;
            JTAG_IR(ir);
          }
          // Read previous data
          do {
            response_value = JTAG_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          post_read = 0U;
        }
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
        // Store previous data
        *response++ = (uint8_t) data;
        *response++ = (uint8_t)(data >>  8);
        *response++ = (uint8_t)(data >> 16);
        *response++ = (uint8_t)(data >> 24);
#if (TIMESTAMP_CLOCK != 0U)
        if (post_read) {
          // Store Timestamp of next AP read
          if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
            timestamp = DAP_Data.timestamp;
            *response++ = (uint8_t) timestamp;
            *response++ = (uint8_t)(timestamp >>  8);
            *response++ = (uint8_t)(timestamp >> 16);
            *response++ = (uint8_t)(timestamp >> 24);
          }
        }
#endif
      }
      if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
        // Read with value match
        match_value = (uint32_t)(*(request+0) <<  0) |
                      (uint32_t)(*(request+1) <<  8) |
                      (uint32_t)(*(request+2) << 16) |
                      (uint32_t)(*(request+3) << 24);
        request += 4;
        match_retry  = DAP_Data.transfer.match_retry;
        // Select JTAG chain
        if (ir != request_ir) {
          ir = request_ir;
          JTAG_IR(ir);
        }
        // Post DP/AP read
        retry = DAP_Data.transfer.retry_count;
        do {
          response_value = JTAG_Transfer(request_value, NULL);
        } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
        do {
          // Read register until its value matches or retry counter expires
          retry = DAP_Data.transfer.retry_count;
          do {
            response_value = JTAG_Transfer(request_value, &data);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if (response_value != DAP_TRANSFER_OK) {
            break;
          }
        } while (((data & DAP_Data.transfer.match_mask) != match_value) && match_retry-- && !DAP_TransferAbort);
        if ((data & DAP_Data.transfer.match_mask) != match_value) {
          response_value |= DAP_TRANSFER_MISMATCH;
        }
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
      } else {
        // Normal read
        if (post_read == 0U) {
          // Select JTAG chain
          if (ir != request_ir) {
            ir = request_ir;
            JTAG_IR(ir);
          }
          // Post DP/AP read
          retry = DAP_Data.transfer.retry_count;
          do {
            response_value = JTAG_Transfer(request_value, NULL);
          } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if (response_value != DAP_TRANSFER_OK) {
            break;
          }
#if (TIMESTAMP_CLOCK != 0U)
          // Store Timestamp
          if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
            timestamp = DAP_Data.timestamp;
            *response++ = (uint8_t) timestamp;
            *response++ = (uint8_t)(timestamp >>  8);
            *response++ = (uint8_t)(timestamp >> 16);
            *response++ = (uint8_t)(timestamp >> 24);
          }
#endif
          post_read = 1U;
        }
      }
    } else {
      // Write register
      if (post_read) {
        // Select JTAG chain
        if (ir != JTAG_DPACC) {
          ir = JTAG_DPACC;
          JTAG_IR(ir);
        }
        // Read previous data
        retry = DAP_Data.transfer.retry_count;
        do {
          response_value = JTAG_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
        } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
        // Store previous data
        *response++ = (uint8_t) data;
        *response++ = (uint8_t)(data >>  8);
        *response++ = (uint8_t)(data >> 16);
        *response++ = (uint8_t)(data >> 24);
        post_read = 0U;
      }
      // Load data
      data = (uint32_t)(*(request+0) <<  0) |
             (uint32_t)(*(request+1) <<  8) |
             (uint32_t)(*(request+2) << 16) |
             (uint32_t)(*(request+3) << 24);
      request += 4;
      if ((request_value & DAP_TRANSFER_MATCH_MASK) != 0U) {
        // Write match mask
        DAP_Data.transfer.match_mask = data;
        response_value = DAP_TRANSFER_OK;
      } else {
        // Select JTAG chain
        if (ir != request_ir) {
          ir = request_ir;
          JTAG_IR(ir);
        }
        // Write DP/AP register
        retry = DAP_Data.transfer.retry_count;
        do {
          response_value = JTAG_Transfer(request_value, &data);
        } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if (response_value != DAP_TRANSFER_OK) {
          break;
        }
#if (TIMESTAMP_CLOCK != 0U)
        // Store Timestamp
        if ((request_value & DAP_TRANSFER_TIMESTAMP) != 0U) {
          timestamp = DAP_Data.timestamp;
          *response++ = (uint8_t) timestamp;
          *response++ = (uint8_t)(timestamp >>  8);
          *response++ = (uint8_t)(timestamp >> 16);
          *response++ = (uint8_t)(timestamp >> 24);
        }
#endif
      }
    }
    response_count++;
    if (DAP_TransferAbort) {
      break;
    }
  }

  for (; request_count != 0U; request_count--) {
    // Process canceled requests
    request_value = *request++;
    if ((request_value & DAP_TRANSFER_RnW) != 0U) {
      // Read register
      if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
        // Read with value match
        request += 4;
      }
    } else {
      // Write register
      request += 4;
    }
  }

  if (response_value == DAP_TRANSFER_OK) {
    // Select JTAG chain
    if (ir != JTAG_DPACC) {
      ir = JTAG_DPACC;
      JTAG_IR(ir);
    }
    if (post_read) {
      // Read previous data
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = JTAG_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
      // Store previous data
      *response++ = (uint8_t) data;
      *response++ = (uint8_t)(data >>  8);
      *response++ = (uint8_t)(data >> 16);
      *response++ = (uint8_t)(data >> 24);
    } else {
      // Check last write
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = JTAG_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
    }
  }

end:
  *(response_head+0) = (uint8_t)response_count;
  *(response_head+1) = (uint8_t)response_value;

  return (((uint32_t)(request - request_head) << 16) | (uint32_t)(response - response_head));
}
#endif


// Process Dummy Transfer command and prepare response
// 处理虚拟传输命令并准备响应
// 当没有连接到有效的调试端口时使用此函数
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (lower 16 bits) (响应字节数，低16位)
//             number of bytes in request (upper 16 bits) (请求字节数，高16位)
static uint32_t DAP_Dummy_Transfer(const uint8_t *request, uint8_t *response) {
  const
  uint8_t  *request_head;   // 保存请求数据的起始指针
  uint32_t  request_count;  // 请求传输的次数
  uint32_t  request_value;  // 当前请求的值

  // 保存请求起始位置，用于计算处理的字节数
  request_head  =  request;

  request++;            // Ignore DAP index (忽略DAP索引)

  // 获取请求次数
  request_count = *request++;

  // 遍历所有请求，但不执行实际操作
  for (; request_count != 0U; request_count--) {
    // Process dummy requests (处理虚拟请求)
    request_value = *request++;
    if ((request_value & DAP_TRANSFER_RnW) != 0U) {
      // Read register (读寄存器操作)
      if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
        // Read with value match (带值匹配的读操作)
        // 跳过4字节的匹配值数据
        request += 4;
      }
    } else {
      // Write register (写寄存器操作)
      // 跳过4字节的写入数据
      request += 4;
    }
  }

  // 设置响应：传输计数为0，响应值为0（表示无操作）
  *(response+0) = 0U;   // Response count (响应计数)
  *(response+1) = 0U;   // Response value (响应值)

  // 返回值：高16位为请求字节数，低16位为响应字节数(2字节)
  return (((uint32_t)(request - request_head) << 16) | 2U);
}


// Process Transfer command and prepare response
// 处理传输命令并准备响应
// 根据当前调试端口类型分发到相应的处理函数
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (lower 16 bits) (响应字节数，低16位)
//             number of bytes in request (upper 16 bits) (请求字节数，高16位)
static uint32_t DAP_Transfer(const uint8_t *request, uint8_t *response) {
  uint32_t num;

  // 根据调试端口类型选择相应的传输函数
  switch (DAP_Data.debug_port) {
#if (DAP_SWD != 0)
    case DAP_PORT_SWD:
      // SWD模式传输
      num = DAP_SWD_Transfer(request, response);
      break;
#endif
#if (DAP_JTAG != 0)
    case DAP_PORT_JTAG:
      // JTAG模式传输
      num = DAP_JTAG_Transfer(request, response);
      break;
#endif
    default:
      // 未知或未连接端口，使用虚拟传输
      num = DAP_Dummy_Transfer(request, response);
      break;
  }

  return (num);
}


// Process SWD Transfer Block command and prepare response
// 处理SWD块传输命令并准备响应
// 用于连续读取或写入多个寄存器值
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (响应字节数)
#if (DAP_SWD != 0)
static uint32_t DAP_SWD_TransferBlock(const uint8_t *request, uint8_t *response) {
  uint32_t  request_count;   // 请求传输的次数
  uint32_t  request_value;   // 请求值（包含寄存器地址和读写标志）
  uint32_t  response_count;  // 成功传输的次数
  uint32_t  response_value;  // 传输响应状态
  uint8_t  *response_head;   // 响应数据的起始指针
  uint32_t  retry;           // 重试计数器
  uint32_t  data;            // 传输的数据

  // 初始化响应计数和响应值
  response_count = 0U;
  response_value = 0U;
  response_head  = response;
  response      += 3;  // 预留3字节用于响应头（2字节计数 + 1字节状态）

  // 清除传输中止标志
  DAP_TransferAbort = 0U;

  request++;            // Ignore DAP index (忽略DAP索引)

  // 获取请求次数（小端序，2字节）
  request_count = (uint32_t)(*(request+0) << 0) |
                  (uint32_t)(*(request+1) << 8);
  request += 2;
  
  // 如果请求次数为0，直接结束
  if (request_count == 0U) {
    goto end;
  }

  // 获取请求值（包含寄存器地址和读写标志）
  request_value = *request++;
  
  if ((request_value & DAP_TRANSFER_RnW) != 0U) {
    // Read register block (读寄存器块)
    if ((request_value & DAP_TRANSFER_APnDP) != 0U) {
      // Post AP read (AP读取需要先发送读请求)
      // AP访问需要两次传输：第一次发送地址，第二次获取数据
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = SWD_Transfer(request_value, NULL);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
    }
    
    // 循环读取所有请求的数据
    while (request_count--) {
      // Read DP/AP register (读取DP/AP寄存器)
      if ((request_count == 0U) && ((request_value & DAP_TRANSFER_APnDP) != 0U)) {
        // Last AP read (最后一次AP读取)
        // 使用RDBUFF寄存器获取最后的AP数据
        request_value = DP_RDBUFF | DAP_TRANSFER_RnW;
      }
      
      // 执行SWD传输，带重试机制
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = SWD_Transfer(request_value, &data);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
      
      // Store data (存储读取的数据，小端序)
      *response++ = (uint8_t) data;
      *response++ = (uint8_t)(data >>  8);
      *response++ = (uint8_t)(data >> 16);
      *response++ = (uint8_t)(data >> 24);
      response_count++;
    }
  } else {
    // Write register block (写寄存器块)
    while (request_count--) {
      // Load data (从请求中加载要写入的数据，小端序)
      data = (uint32_t)(*(request+0) <<  0) |
             (uint32_t)(*(request+1) <<  8) |
             (uint32_t)(*(request+2) << 16) |
             (uint32_t)(*(request+3) << 24);
      request += 4;
      
      // Write DP/AP register (写入DP/AP寄存器)
      // 执行SWD传输，带重试机制
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = SWD_Transfer(request_value, &data);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
      response_count++;
    }
    
    // Check last write (检查最后一次写入是否成功)
    // 通过读取RDBUFF寄存器来确认写入完成
    retry = DAP_Data.transfer.retry_count;
    do {
      response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
    } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
  }

end:
  // 填充响应头
  // 响应计数（2字节，小端序）
  *(response_head+0) = (uint8_t)(response_count >> 0);
  *(response_head+1) = (uint8_t)(response_count >> 8);
  // 响应状态值
  *(response_head+2) = (uint8_t) response_value;

  // 返回响应数据的总字节数
  return ((uint32_t)(response - response_head));
}
#endif

// Process JTAG Transfer Block command and prepare response
// 处理JTAG块传输命令并准备响应
// 用于通过JTAG接口连续读取或写入多个寄存器值
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (响应字节数)
#if (DAP_JTAG != 0)
static uint32_t DAP_JTAG_TransferBlock(const uint8_t *request, uint8_t *response) {
  uint32_t  request_count;   // 请求传输的次数
  uint32_t  request_value;   // 请求值（包含寄存器地址和读写标志）
  uint32_t  response_count;  // 成功传输的次数
  uint32_t  response_value;  // 传输响应状态
  uint8_t  *response_head;   // 响应数据的起始指针
  uint32_t  retry;           // 重试计数器
  uint32_t  data;            // 传输的数据
  uint32_t  ir;              // JTAG指令寄存器值

  // 初始化响应计数和响应值
  response_count = 0U;
  response_value = 0U;
  response_head  = response;
  response      += 3;  // 预留3字节用于响应头（2字节计数 + 1字节状态）

  // 清除传输中止标志
  DAP_TransferAbort = 0U;

  // Device index (JTAG TAP)
  // 设备索引（JTAG TAP链中的设备位置）
  DAP_Data.jtag_dev.index = *request++;
  if (DAP_Data.jtag_dev.index >= DAP_Data.jtag_dev.count) {
    // 设备索引超出范围，直接结束
    goto end;
  }

  // 获取请求次数（小端序，2字节）
  request_count = (uint32_t)(*(request+0) << 0) |
                  (uint32_t)(*(request+1) << 8);
  request += 2;
  
  // 如果请求次数为0，直接结束
  if (request_count == 0U) {
    goto end;
  }

  // 获取请求值（包含寄存器地址和读写标志）
  request_value = *request++;

  // Select JTAG chain (选择JTAG链)
  // 根据访问类型选择AP访问或DP访问指令
  ir = (request_value & DAP_TRANSFER_APnDP) ? JTAG_APACC : JTAG_DPACC;
  JTAG_IR(ir);

  if ((request_value & DAP_TRANSFER_RnW) != 0U) {
    // Read register block (读寄存器块)
    // Post read (先发送读请求)
    // JTAG访问需要两次传输：第一次发送地址，第二次获取数据
    retry = DAP_Data.transfer.retry_count;
    do {
      response_value = JTAG_Transfer(request_value, NULL);
    } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
    if (response_value != DAP_TRANSFER_OK) {
      goto end;
    }
    
    // 循环读取所有请求的数据
    while (request_count--) {
      // Read DP/AP register (读取DP/AP寄存器)
      if (request_count == 0U) {
        // Last read (最后一次读取)
        // 如果当前不在DPACC模式，切换到DPACC
        if (ir != JTAG_DPACC) {
          JTAG_IR(JTAG_DPACC);
        }
        // 使用RDBUFF寄存器获取最后的AP数据
        request_value = DP_RDBUFF | DAP_TRANSFER_RnW;
      }
      
      // 执行JTAG传输，带重试机制
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = JTAG_Transfer(request_value, &data);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
      
      // Store data (存储读取的数据，小端序)
      *response++ = (uint8_t) data;
      *response++ = (uint8_t)(data >>  8);
      *response++ = (uint8_t)(data >> 16);
      *response++ = (uint8_t)(data >> 24);
      response_count++;
    }
  } else {
    // Write register block (写寄存器块)
    while (request_count--) {
      // Load data (从请求中加载要写入的数据，小端序)
      data = (uint32_t)(*(request+0) <<  0) |
             (uint32_t)(*(request+1) <<  8) |
             (uint32_t)(*(request+2) << 16) |
             (uint32_t)(*(request+3) << 24);
      request += 4;
      
      // Write DP/AP register (写入DP/AP寄存器)
      // 执行JTAG传输，带重试机制
      retry = DAP_Data.transfer.retry_count;
      do {
        response_value = JTAG_Transfer(request_value, &data);
      } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if (response_value != DAP_TRANSFER_OK) {
        goto end;
      }
      response_count++;
    }
    
    // Check last write (检查最后一次写入是否成功)
    // 如果当前不在DPACC模式，切换到DPACC
    if (ir != JTAG_DPACC) {
      JTAG_IR(JTAG_DPACC);
    }
    // 通过读取RDBUFF寄存器来确认写入完成
    retry = DAP_Data.transfer.retry_count;
    do {
      response_value = JTAG_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
    } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
  }

end:
  // 填充响应头
  // 响应计数（2字节，小端序）
  *(response_head+0) = (uint8_t)(response_count >> 0);
  *(response_head+1) = (uint8_t)(response_count >> 8);
  // 响应状态值
  *(response_head+2) = (uint8_t) response_value;

  // 返回响应数据的总字节数
  return ((uint32_t)(response - response_head));
}
#endif


// Process Transfer Block command and prepare response
// 处理块传输命令并准备响应
// 根据当前调试端口类型（SWD或JTAG）调用相应的块传输函数
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (lower 16 bits) (响应字节数，低16位)
//             number of bytes in request (upper 16 bits) (请求字节数，高16位)
static uint32_t DAP_TransferBlock(const uint8_t *request, uint8_t *response) {
  uint32_t num;

  // 根据当前调试端口类型选择相应的处理函数
  switch (DAP_Data.debug_port) {
#if (DAP_SWD != 0)
    case DAP_PORT_SWD:
      num = DAP_SWD_TransferBlock (request, response);
      break;
#endif
#if (DAP_JTAG != 0)
    case DAP_PORT_JTAG:
      num = DAP_JTAG_TransferBlock(request, response);
      break;
#endif
    default:
      // 未知端口类型，返回错误响应
      *(response+0) = 0U;       // Response count [7:0] (响应计数低字节)
      *(response+1) = 0U;       // Response count[15:8] (响应计数高字节)
      *(response+2) = 0U;       // Response value (响应状态值)
      num = 3U;
      break;
  }

  // 计算请求数据的字节数并存入高16位
  if ((*(request+3) & DAP_TRANSFER_RnW) != 0U) {
    // Read register block (读寄存器块)
    // 读操作请求数据固定为4字节（索引+计数+请求值）
    num |=  4U << 16;
  } else {
    // Write register block (写寄存器块)
    // 写操作请求数据为4字节头部 + (传输次数 * 4字节数据)
    num |= (4U + (((uint32_t)(*(request+1)) | (uint32_t)(*(request+2) << 8)) * 4)) << 16;
  }

  return (num);
}


// Process SWD Write ABORT command and prepare response
// 处理SWD写中止命令并准备响应
// 用于向SWD调试端口写入ABORT寄存器以清除错误状态
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (响应字节数)
#if (DAP_SWD != 0)
static uint32_t DAP_SWD_WriteAbort(const uint8_t *request, uint8_t *response) {
  uint32_t data;

  // Load data (Ignore DAP index)
  // 加载要写入ABORT寄存器的数据（忽略DAP索引，从第2字节开始）
  // 数据格式为小端序，4字节
  data = (uint32_t)(*(request+1) <<  0) |
         (uint32_t)(*(request+2) <<  8) |
         (uint32_t)(*(request+3) << 16) |
         (uint32_t)(*(request+4) << 24);

  // Write Abort register (写入ABORT寄存器)
  // 清除调试端口的错误状态
  SWD_Transfer(DP_ABORT, &data);

  *response = DAP_OK;
  return (1U);
}
#endif


// Process JTAG Write ABORT command and prepare response
// 处理JTAG写中止命令并准备响应
// 用于向JTAG调试端口写入ABORT寄存器以清除错误状态
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (响应字节数)
#if (DAP_JTAG != 0)
static uint32_t DAP_JTAG_WriteAbort(const uint8_t *request, uint8_t *response) {
  uint32_t data;

  // Device index (JTAG TAP)
  // 设备索引（JTAG TAP链中的设备位置）
  DAP_Data.jtag_dev.index = *request;
  if (DAP_Data.jtag_dev.index >= DAP_Data.jtag_dev.count) {
    // 设备索引超出范围，返回错误
    *response = DAP_ERROR;
    return (1U);
  }

  // Select JTAG chain (选择JTAG链)
  // 设置JTAG指令寄存器为ABORT指令
  JTAG_IR(JTAG_ABORT);

  // Load data (加载要写入ABORT寄存器的数据)
  // 数据格式为小端序，4字节
  data = (uint32_t)(*(request+1) <<  0) |
         (uint32_t)(*(request+2) <<  8) |
         (uint32_t)(*(request+3) << 16) |
         (uint32_t)(*(request+4) << 24);

  // Write Abort register (写入ABORT寄存器)
  // 清除调试端口的错误状态
  JTAG_WriteAbort(data);

  *response = DAP_OK;
  return (1U);
}
#endif


// Process Write ABORT command and prepare response
// 处理写中止命令并准备响应
// 根据当前调试端口类型（SWD或JTAG）调用相应的写中止函数
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (lower 16 bits) (响应字节数，低16位)
//             number of bytes in request (upper 16 bits) (请求字节数，高16位)
static uint32_t DAP_WriteAbort(const uint8_t *request, uint8_t *response) {
  uint32_t num;

  // 根据当前调试端口类型选择相应的处理函数
  switch (DAP_Data.debug_port) {
#if (DAP_SWD != 0)
    case DAP_PORT_SWD:
      num = DAP_SWD_WriteAbort (request, response);
      break;
#endif
#if (DAP_JTAG != 0)
    case DAP_PORT_JTAG:
      num = DAP_JTAG_WriteAbort(request, response);
      break;
#endif
    default:
      // 未知端口类型，返回错误
      *response = DAP_ERROR;
      num = 1U;
      break;
  }
  // 请求数据固定为5字节（索引 + 4字节ABORT数据）
  return ((5U << 16) | num);
}

// Process DAP command request and prepare response
// 处理DAP命令请求并准备响应
// 这是DAP协议的核心命令分发函数，根据命令ID调用相应的处理函数
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (lower 16 bits) (响应字节数，低16位)
//             number of bytes in request (upper 16 bits) (请求字节数，高16位)
uint32_t DAP_ProcessCommand(const uint8_t *request, uint8_t *response) {
  uint32_t num;

  // 检查是否为厂商自定义命令 (ID_DAP_Vendor0 到 ID_DAP_Vendor31)
  if ((*request >= ID_DAP_Vendor0) && (*request <= ID_DAP_Vendor31)) {
    return DAP_ProcessVendorCommand(request, response);
  }

  // 将命令ID复制到响应缓冲区（响应的第一个字节总是命令ID）
  *response++ = *request;

  // 根据命令ID分发到相应的处理函数
  switch (*request++) {
    case ID_DAP_Info:
      // 获取DAP信息（固件版本、设备能力等）
      num = DAP_Info(*request, response+1);
      *response = (uint8_t)num;
      return ((2U << 16) + 2U + num);

    case ID_DAP_HostStatus:
      // 设置主机状态（LED指示等）
      num = DAP_HostStatus(request, response);
      break;

    case ID_DAP_Connect:
      // 连接到目标设备（选择SWD或JTAG模式）
      num = DAP_Connect(request, response);
      break;
    case ID_DAP_Disconnect:
      // 断开与目标设备的连接
      num = DAP_Disconnect(response);
      break;

    case ID_DAP_Delay:
      // 执行延时操作
      num = DAP_Delay(request, response);
      break;

    case ID_DAP_ResetTarget:
      // 复位目标设备
      num = DAP_ResetTarget(response);
      break;

    case ID_DAP_SWJ_Pins:
      // 控制SWJ引脚状态（SWCLK/TCK, SWDIO/TMS, TDI, TDO, nTRST, nRESET）
      num = DAP_SWJ_Pins(request, response);
      break;
    case ID_DAP_SWJ_Clock:
      // 设置SWJ时钟频率
      num = DAP_SWJ_Clock(request, response);
      break;
    case ID_DAP_SWJ_Sequence:
      // 发送SWJ序列（用于SWD/JTAG模式切换）
      num = DAP_SWJ_Sequence(request, response);
      break;

    case ID_DAP_SWD_Configure:
      // 配置SWD协议参数（周转周期、数据相位）
      num = DAP_SWD_Configure(request, response);
      break;
    case ID_DAP_SWD_Sequence:
      // 发送SWD序列
      num = DAP_SWD_Sequence(request, response);
      break;

    case ID_DAP_JTAG_Sequence:
      // 发送JTAG序列
      num = DAP_JTAG_Sequence(request, response);
      break;
    case ID_DAP_JTAG_Configure:
      // 配置JTAG链（设备数量和IR长度）
      num = DAP_JTAG_Configure(request, response);
      break;
    case ID_DAP_JTAG_IDCODE:
      // 读取JTAG设备IDCODE
      num = DAP_JTAG_IDCode(request, response);
      break;

    case ID_DAP_TransferConfigure:
      // 配置传输参数（空闲周期、重试次数等）
      num = DAP_TransferConfigure(request, response);
      break;
    case ID_DAP_Transfer:
      // 执行DAP传输（读写调试寄存器）
      num = DAP_Transfer(request, response);
      break;
    case ID_DAP_TransferBlock:
      // 执行DAP块传输（批量读写）
      num = DAP_TransferBlock(request, response);
      break;

    case ID_DAP_WriteABORT:
      // 写入ABORT寄存器（清除错误状态）
      num = DAP_WriteAbort(request, response);
      break;

#if ((SWO_UART != 0) || (SWO_MANCHESTER != 0))
    // SWO (Serial Wire Output) 相关命令
    case ID_DAP_SWO_Transport:
      // 配置SWO传输模式
      num = SWO_Transport(request, response);
      break;
    case ID_DAP_SWO_Mode:
      // 设置SWO模式（UART或Manchester编码）
      num = SWO_Mode(request, response);
      break;
    case ID_DAP_SWO_Baudrate:
      // 设置SWO波特率
      num = SWO_Baudrate(request, response);
      break;
    case ID_DAP_SWO_Control:
      // 控制SWO捕获（启动/停止）
      num = SWO_Control(request, response);
      break;
    case ID_DAP_SWO_Status:
      // 获取SWO状态
      num = SWO_Status(response);
      break;
    case ID_DAP_SWO_ExtendedStatus:
      // 获取SWO扩展状态信息
      num = SWO_ExtendedStatus(request, response);
      break;
    case ID_DAP_SWO_Data:
      // 读取SWO数据
      num = SWO_Data(request, response);
      break;
#endif

#if (DAP_UART != 0)
    // UART通信相关命令
    case ID_DAP_UART_Transport:
      // 配置UART传输
      num = UART_Transport(request, response);
      break;
    case ID_DAP_UART_Configure:
      // 配置UART参数（波特率、数据位等）
      num = UART_Configure(request, response);
      break;
    case ID_DAP_UART_Control:
      // 控制UART（启动/停止）
      num = UART_Control(request, response);
      break;
    case ID_DAP_UART_Status:
      // 获取UART状态
      num = UART_Status(response);
      break;
    case ID_DAP_UART_Transfer:
      // UART数据传输
      num = UART_Transfer(request, response);
      break;
#endif

    default:
      // 无效命令，返回ID_DAP_Invalid
      *(response-1) = ID_DAP_Invalid;
      return ((1U << 16) | 1U);
  }

  // 返回值：请求1字节 + 响应(1字节命令ID + num字节数据)
  return ((1U << 16) + 1U + num);
}


// Execute DAP command (process request and prepare response)
// 执行DAP命令（处理请求并准备响应）
// 支持单个命令和批量命令（ID_DAP_ExecuteCommands）的执行
//   request:  pointer to request data (指向请求数据的指针)
//   response: pointer to response data (指向响应数据的指针)
//   return:   number of bytes in response (lower 16 bits) (响应字节数，低16位)
//             number of bytes in request (upper 16 bits) (请求字节数，高16位)
uint32_t DAP_ExecuteCommand(const uint8_t *request, uint8_t *response) {
  uint32_t cnt, num, n;

  // 检查是否为批量执行命令
  if (*request == ID_DAP_ExecuteCommands) {
    // 复制命令ID到响应
    *response++ = *request++;
    // 获取要执行的命令数量
    cnt = *request++;
    // 将命令数量写入响应
    *response++ = (uint8_t)cnt;
    // 初始化计数：请求2字节（命令ID+数量），响应2字节
    num = (2U << 16) | 2U;
    // 循环处理每个命令
    while (cnt--) {
      // 处理单个命令
      n = DAP_ProcessCommand(request, response);
      // 累加请求和响应字节数
      num += n;
      // 移动请求指针到下一个命令
      request  += (uint16_t)(n >> 16);
      // 移动响应指针到下一个响应位置
      response += (uint16_t) n;
    }
    return (num);
  }

  // 单个命令直接处理
  return DAP_ProcessCommand(request, response);
}


// Setup DAP
// 初始化DAP模块
// 设置默认配置参数，包括调试端口、时钟、传输参数等
void DAP_Setup(void) {

  // Default settings (默认设置)
  DAP_Data.debug_port  = 0U;                              // 调试端口类型（未连接）
  DAP_Data.fast_clock  = 0U;                              // 快速时钟模式禁用
  DAP_Data.clock_delay = CLOCK_DELAY(DAP_DEFAULT_SWJ_CLOCK); // 默认时钟延时
  DAP_Data.transfer.idle_cycles = 0U;                     // 传输后空闲周期数
  DAP_Data.transfer.retry_count = 100U;                   // 传输重试次数
  DAP_Data.transfer.match_retry = 0U;                     // 匹配重试次数
  DAP_Data.transfer.match_mask  = 0x00000000U;            // 匹配掩码
#if (DAP_SWD != 0)
  DAP_Data.swd_conf.turnaround  = 1U;                     // SWD周转周期（默认1个时钟）
  DAP_Data.swd_conf.data_phase  = 0U;                     // SWD数据相位（禁用额外数据相位）
#endif
#if (DAP_JTAG != 0)
  DAP_Data.jtag_dev.count = 0U;                           // JTAG链中设备数量
#endif

  DAP_SETUP();  // Device specific setup (设备特定初始化)
}
