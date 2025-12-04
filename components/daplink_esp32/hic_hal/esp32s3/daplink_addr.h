/**
 * @file    daplink_addr.h
 * @brief   DAPLink 内存地址定义 (ESP32-S3)
 * 
 * @author  星年
 * @date    2025-12-04
 */

#ifndef DAPLINK_ADDR_H
#define DAPLINK_ADDR_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== ESP32-S3 内存映射 ==================== */

// ESP32-S3 SRAM 起始地址和大小
#define DAPLINK_RAM_START       0x3FC88000
#define DAPLINK_RAM_SIZE        (512 * 1024)    // 512KB

// ESP32-S3 Flash 起始地址和大小
#define DAPLINK_ROM_START       0x42000000
#define DAPLINK_ROM_SIZE        (16 * 1024 * 1024)  // 16MB

/* ==================== DAPLink 应用区域 ==================== */

// 应用程序 RAM 区域
#define DAPLINK_RAM_APP_START   DAPLINK_RAM_START
#define DAPLINK_RAM_APP_SIZE    (480 * 1024)    // 480KB 给应用

// 共享 RAM 区域（用于数据交换）
#define DAPLINK_RAM_SHARED_START    (DAPLINK_RAM_APP_START + DAPLINK_RAM_APP_SIZE)
#define DAPLINK_RAM_SHARED_SIZE     (32 * 1024)     // 32KB 共享区

/* ==================== DAPLink 配置区域 ==================== */

// 配置数据存储地址（Flash 中）
#define DAPLINK_CONFIG_START    (DAPLINK_ROM_START + 0x10000)
#define DAPLINK_CONFIG_SIZE     (4 * 1024)      // 4KB

/* ==================== 栈和堆配置 ==================== */

// 主任务栈大小
#define DAPLINK_MAIN_STACK_SIZE     (8 * 1024)

// DAP 任务栈大小
#define DAPLINK_DAP_STACK_SIZE      (8 * 1024)

// USB 任务栈大小
#define DAPLINK_USB_STACK_SIZE      (4 * 1024)

#ifdef __cplusplus
}
#endif

#endif /* DAPLINK_ADDR_H */
