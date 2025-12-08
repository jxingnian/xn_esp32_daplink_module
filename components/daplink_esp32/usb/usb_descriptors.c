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

// 配置描述符总长度 (9 + 9 + 7 + 7 = 32)
#define CONFIG_TOTAL_LEN  (9 + 9 + 7 + 7)

// 配置描述符
const uint8_t desc_fs_configuration[] __attribute__((used)) = {
    // 配置描述符头
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // CMSIS-DAP v2 接口描述符
    // 使用 Vendor 类,但需要 WinUSB 兼容 ID
    // 接口描述符
    9, TUSB_DESC_INTERFACE, ITF_NUM_VENDOR, 0, 2, 0xFF, 0x00, 0x00, 0,
    
    // Bulk OUT 端点 (Full Speed 最大 64 字节)
    7, TUSB_DESC_ENDPOINT, EPNUM_VENDOR_OUT, TUSB_XFER_BULK, U16_TO_U8S_LE(64), 0,
    
    // Bulk IN 端点 (Full Speed 最大 64 字节)
    7, TUSB_DESC_ENDPOINT, EPNUM_VENDOR_IN, TUSB_XFER_BULK, U16_TO_U8S_LE(64), 0
};

// Microsoft OS 2.0 描述符长度
#define MS_OS_20_DESC_LEN  0xB2

// BOS 描述符 (Binary Object Store)
#define BOS_TOTAL_LEN      (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

const uint8_t desc_bos[] __attribute__((used)) = {
    // BOS 头
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    
    // Microsoft OS 2.0 描述符
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, 1)
};

// Microsoft OS 2.0 描述符集

const uint8_t desc_ms_os_20[] __attribute__((used)) = {
    // Set header: length, type, windows version, total length
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
    
    // Configuration subset header: length, type, configuration index, reserved, configuration total length
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),
    
    // Function subset header: length, type, first interface, reserved, subset length
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_VENDOR, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),
    
    // Compatible ID descriptor: length, type, compatible ID, sub compatible ID
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // Registry property descriptor: length, type
    U16_TO_U8S_LE(0x0084), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength
    // Property name: DeviceInterfaceGUIDs
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    U16_TO_U8S_LE(0x0050), // wPropertyDataLength
    // Property data: {CDB3B5AD-293B-4663-AA36-1AAE46463776}
    '{', 0, 'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0, '2', 0, '9', 0, '3', 0, 'B', 0, '-', 0, '4', 0, '6', 0, '6', 0, '3', 0, '-', 0, 'A', 0, 'A', 0, '3', 0, '6', 0, '-', 0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0, '6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0, '}', 0, 0, 0, 0, 0
};
