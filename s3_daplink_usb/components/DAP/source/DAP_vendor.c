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
 * Title:        DAP_vendor.c CMSIS-DAP Vendor Commands
 *               CMSIS-DAP 厂商自定义命令处理
 *
 *---------------------------------------------------------------------------*/

#include "DAP_config.h"
#include "DAP.h"

//**************************************************************************************************
/**
\defgroup DAP_Vendor_Adapt_gr Adapt Vendor Commands
\ingroup DAP_Vendor_gr
@{

DAP_vendor.c 文件提供了用于扩展调试单元厂商自定义命令的模板源代码。
将此文件复制到调试单元的项目文件夹中，并将该文件添加到 MDK-ARM 项目的
Configuration 文件组下。

厂商命令ID范围: ID_DAP_Vendor0 (0x80) 到 ID_DAP_Vendor31 (0x9F)
共32个可用的厂商自定义命令槽位。
*/

/** 处理 DAP 厂商自定义命令并准备响应数据
\param request   指向请求数据的指针
\param response  指向响应数据的指针
\return          响应字节数（低16位）
                 请求字节数（高16位）

返回值格式说明:
- 低16位: 响应数据的字节数
- 高16位: 已处理的请求数据字节数
- 初始值 (1U << 16) | 1U 表示: 请求1字节(命令ID) + 响应1字节(命令ID回显)
*/
uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response) {
  // 初始化计数: 请求1字节(命令ID)，响应1字节(命令ID回显)
  uint32_t num = (1U << 16) | 1U;

  // 将命令ID复制到响应缓冲区（响应的第一个字节总是命令ID）
  *response++ = *request;

  // 根据命令ID分发到相应的处理逻辑
  // 请求的第一个字节是命令ID
  switch (*request++) {
    case ID_DAP_Vendor0:
#if 0                            // 用户命令示例
      num += 1U << 16;           // 增加请求计数（表示多读取了1字节请求数据）
      if (*request == 1U) {      // 当第一个命令数据字节为1时
        *response++ = 'X';       // 发送 'X' 作为响应
        num++;                   // 增加响应计数
      }
#endif
      break;

    // 以下为保留的厂商命令槽位，可根据需要实现自定义功能
    case ID_DAP_Vendor1:  break;
    case ID_DAP_Vendor2:  break;
    case ID_DAP_Vendor3:  break;
    case ID_DAP_Vendor4:  break;
    case ID_DAP_Vendor5:  break;
    case ID_DAP_Vendor6:  break;
    case ID_DAP_Vendor7:  break;
    case ID_DAP_Vendor8:  break;
    case ID_DAP_Vendor9:  break;
    case ID_DAP_Vendor10: break;
    case ID_DAP_Vendor11: break;
    case ID_DAP_Vendor12: break;
    case ID_DAP_Vendor13: break;
    case ID_DAP_Vendor14: break;
    case ID_DAP_Vendor15: break;
    case ID_DAP_Vendor16: break;
    case ID_DAP_Vendor17: break;
    case ID_DAP_Vendor18: break;
    case ID_DAP_Vendor19: break;
    case ID_DAP_Vendor20: break;
    case ID_DAP_Vendor21: break;
    case ID_DAP_Vendor22: break;
    case ID_DAP_Vendor23: break;
    case ID_DAP_Vendor24: break;
    case ID_DAP_Vendor25: break;
    case ID_DAP_Vendor26: break;
    case ID_DAP_Vendor27: break;
    case ID_DAP_Vendor28: break;
    case ID_DAP_Vendor29: break;
    case ID_DAP_Vendor30: break;
    case ID_DAP_Vendor31: break;
  }

  return (num);
}

///@}
