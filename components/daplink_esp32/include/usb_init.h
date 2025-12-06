/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-06 13:53:48
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-06 13:54:11
 * @FilePath: \todo-xn_esp32_daplink_module\components\daplink_esp32\include\usb_init.h
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
/**
 * @file usb_init.h
 * @brief USB 初始化接口
 * @author 星年
 * @date 2025-12-06
 */

#ifndef USB_INIT_H
#define USB_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 USB 设备
 * @return 0 成功，-1 失败
 */
int usb_init(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_INIT_H */
