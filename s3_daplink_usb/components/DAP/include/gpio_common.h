/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-08 21:18:04
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-08 21:41:53
 * @FilePath: \todo-xn_esp32_daplink_module\s3_daplink_usb\components\DAP\include\gpio_common.h
 * @Description: GPIO通用头文件 - ESP32-S3 + ESP-IDF 5.5 专用
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef __GPIO_COMMON_H__
#define __GPIO_COMMON_H__

#include "sdkconfig.h"

#ifndef CONFIG_IDF_TARGET_ESP32S3
    #error "This project only supports ESP32-S3"
#endif

#include "soc/gpio_struct.h"
#include "hal/gpio_ll.h"
#include "hal/clk_gate_ll.h"
#include "soc/dport_access.h"
#include "soc/periph_defs.h"
#include "soc/usb_serial_jtag_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_struct.h"
#include "soc/spi_reg.h"

#endif /* __GPIO_COMMON_H__ */