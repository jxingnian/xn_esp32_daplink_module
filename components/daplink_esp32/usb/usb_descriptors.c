/**
 * @file usb_descriptors.c
 * @brief USB Vendor 类描述符（CMSIS-DAP v2 Bulk 传输）
 * @author 星年
 * @date 2025-12-07
 */

#include <stdint.h>
#include <string.h>
#include "tusb.h"

// USB 设备描述符
const tusb_desc_device_t desc_device __attribute__((used)) = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x0D28,  // ARM Ltd
    .idProduct          = 0x0204,  // DAPLink CMSIS-DAP
    .bcdDevice          = 0x0200,  // v2.0
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// 接口编号
enum {
    ITF_NUM_VENDOR = 0,
    ITF_NUM_TOTAL
};

// 端点地址
#define EPNUM_VENDOR_OUT   0x01
#define EPNUM_VENDOR_IN    0x81

// 配置描述符总长度
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

// 配置描述符
const uint8_t desc_fs_configuration[] __attribute__((used)) = {
    // 配置描述符头
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Vendor 接口描述符（Bulk 端点）
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64)
};
