/**
 * @file usb_descriptors.c
 * @brief USB 描述符定义文件 - CMSIS-DAP v2 设备
 * 
 * 本文件定义了 USB 设备所需的所有描述符，包括：
 * 1. 设备描述符 (Device Descriptor) - 设备基本信息
 * 2. 配置描述符 (Configuration Descriptor) - 接口和端点配置
 * 3. BOS 描述符 (Binary Object Store) - USB 2.1 扩展能力
 * 4. MS OS 2.0 描述符 - Windows 自动加载 WinUSB 驱动
 * 5. 字符串描述符 - 厂商名、产品名、序列号
 * 
 * USB 枚举流程：
 * 1. 主机发送 GET_DESCRIPTOR(DEVICE) -> 返回 desc_device
 * 2. 主机发送 GET_DESCRIPTOR(CONFIG) -> 返回 desc_fs_configuration
 * 3. 主机发送 GET_DESCRIPTOR(STRING) -> 调用 tud_descriptor_string_cb()
 * 4. 主机发送 GET_DESCRIPTOR(BOS)    -> 调用 tud_descriptor_bos_cb()
 * 5. Windows 发送 Vendor Request     -> 调用 tud_vendor_control_xfer_cb()
 */

#include <string.h>
#include <stdio.h>
#include "tusb.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "USB_DESC";

// 动态序列号缓冲区（基于芯片唯一 MAC 地址）
static char serial_number[17];  // "XXXXXXXXXXXX" + '\0'

// ==========================================================================
// USB 设备标识符
// ==========================================================================

/**
 * USB 厂商 ID (Vendor ID)
 * 0x0D28 是 ARM DAPLink 官方 VID
 * Keil 等调试软件会自动识别此 VID
 */
#define USB_VID   0x0D28

/**
 * USB 产品 ID (Product ID)
 * 0x0204 是 DAPLink 官方 PID
 */
#define USB_PID   0x0204

// ==========================================================================
// 接口和端点定义
// ==========================================================================

/**
 * USB 接口编号枚举
 * CMSIS-DAP v2 只需要一个 Vendor 类接口
 */
enum {
    ITF_NUM_VENDOR = 0,  // Vendor 接口编号
    ITF_NUM_TOTAL        // 接口总数 = 1
};

/**
 * 端点地址定义
 * 
 * USB 端点地址格式：bit[7]=方向(0=OUT,1=IN), bit[3:0]=端点号
 * - 0x01: 端点1 OUT (主机->设备) - 接收 DAP 命令
 * - 0x81: 端点1 IN  (设备->主机) - 发送 DAP 响应
 */
#define EPNUM_VENDOR_OUT   0x01  // Bulk OUT 端点
#define EPNUM_VENDOR_IN    0x81  // Bulk IN 端点

/**
 * 配置描述符总长度
 * = 配置描述符头(9字节) + Vendor接口描述符(9+7+7=23字节) = 32字节
 */
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

// ==========================================================================
// 设备描述符 (Device Descriptor)
// 
// USB 枚举时主机首先请求的描述符，包含设备基本信息
// 长度固定 18 字节
// ==========================================================================

const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),  // 描述符长度 = 18
    .bDescriptorType    = TUSB_DESC_DEVICE,            // 描述符类型 = 0x01 (设备)
    .bcdUSB             = 0x0210,  // USB 版本 2.1 (支持 BOS 描述符)
    .bDeviceClass       = 0x00,    // 设备类在接口中定义
    .bDeviceSubClass    = 0x00,    // 子类
    .bDeviceProtocol    = 0x00,    // 协议
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,  // 端点0最大包大小 (64字节)
    .idVendor           = USB_VID,   // 厂商 ID
    .idProduct          = USB_PID,   // 产品 ID
    .bcdDevice          = 0x0200,    // 设备版本 2.00
    .iManufacturer      = 0x01,      // 厂商字符串索引 -> "XingNian"
    .iProduct           = 0x02,      // 产品字符串索引 -> "CMSIS-DAP v2"
    .iSerialNumber      = 0x03,      // 序列号字符串索引 -> "123456"
    .bNumConfigurations = 0x01       // 配置数量 = 1
};

// ==========================================================================
// 配置描述符 (Configuration Descriptor)
// 
// 描述设备的接口和端点配置
// 包含：配置描述符头 + 接口描述符 + 端点描述符
// ==========================================================================

const uint8_t desc_fs_configuration[] = {
    /**
     * 配置描述符头 (9 字节)
     * 参数：配置号, 接口数, 字符串索引, 总长度, 属性, 最大电流(2mA单位)
     * 100 = 200mA 最大功耗
     */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    /**
     * Vendor 类接口描述符 (23 字节)
     * 参数：接口号, 字符串索引, OUT端点, IN端点, 最大包大小
     * 
     * 展开后包含：
     * - 接口描述符 (9字节): bInterfaceClass = 0xFF (Vendor)
     * - Bulk OUT 端点描述符 (7字节)
     * - Bulk IN 端点描述符 (7字节)
     */
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT,
                          EPNUM_VENDOR_IN, 64),
};

// ==========================================================================
// MS OS 2.0 描述符
// 
// 作用：让 Windows 自动加载 WinUSB 驱动，无需手动安装 .inf 文件
// 
// 工作原理：
// 1. 设备在 BOS 描述符中声明支持 MS OS 2.0
// 2. Windows 检测到后，发送 Vendor Request 请求 MS OS 2.0 描述符
// 3. 设备返回 desc_ms_os_20，其中声明兼容 "WINUSB"
// 4. Windows 自动加载 WinUSB 驱动
// ==========================================================================

#define MS_OS_20_DESC_LEN   0xA2  // MS OS 2.0 描述符总长度 = 162 字节
#define BOS_TOTAL_LEN       0x21  // BOS 描述符总长度 = 33 字节
#define MS_VENDOR_CODE      0x01  // Vendor Request 代码，Windows 用此代码请求描述符
#define MS_OS_20_WINDEX     7     // wIndex = 7 表示请求 MS OS 2.0 描述符

/** 辅助宏：将 16 位值转换为小端序两字节 */
#define USBShort(v)   ((uint8_t)((v) & 0xFF)), ((uint8_t)(((v) >> 8) & 0xFF))

/**
 * Microsoft OS 2.0 描述符集
 * 
 * 包含三部分：
 * 1. 描述符集头部 (10 字节) - 声明 Windows 版本和总长度
 * 2. 兼容 ID 描述符 (20 字节) - 声明兼容 "WINUSB" 驱动
 * 3. 注册表属性描述符 (132 字节) - 设置设备接口 GUID
 */
const uint8_t desc_ms_os_20[MS_OS_20_DESC_LEN] = {
    // ========== 描述符集头部 (10 字节) ==========
    0x0A, 0x00,                  // wLength = 10
    0x00, 0x00,                  // wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR
    0x00, 0x00, 0x03, 0x06,      // dwWindowsVersion = 0x06030000 (Windows 8.1+)
    USBShort(MS_OS_20_DESC_LEN), // wTotalLength = 162

    // ========== 兼容 ID 描述符 (20 字节) ==========
    // 告诉 Windows 此设备兼容 WinUSB 驱动
    0x14, 0x00,                  // wLength = 20
    USBShort(0x0003),            // wDescriptorType = MS_OS_20_FEATURE_COMPATIBLE_ID
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,  // compatibleID = "WINUSB"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // subCompatibleID (空)

    // ========== 注册表属性描述符 (132 字节) ==========
    // 设置设备接口 GUID，应用程序通过此 GUID 打开设备
    0x84, 0x00,                  // wLength = 132
    USBShort(0x0004),            // wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY
    0x07, 0x00,                  // wPropertyDataType = REG_MULTI_SZ
    0x2A, 0x00,                  // wPropertyNameLength = 42
    // 属性名: "DeviceInterfaceGUIDs" (UTF-16LE)
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,
    'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,
    0x50, 0x00,                  // wPropertyDataLength = 80
    // 属性值: GUID "{CDB3B5AD-293B-4663-AA36-1AAE46463776}" (UTF-16LE)
    // 这是 CMSIS-DAP 的标准 GUID，调试软件通过此 GUID 找到设备
    '{',0,'C',0,'D',0,'B',0,'3',0,'B',0,'5',0,'A',0,'D',0,'-',0,
    '2',0,'9',0,'3',0,'B',0,'-',0,'4',0,'6',0,'6',0,'3',0,'-',0,
    'A',0,'A',0,'3',0,'6',0,'-',0,'1',0,'A',0,'A',0,'E',0,'4',0,
    '6',0,'4',0,'6',0,'3',0,'7',0,'7',0,'6',0,'}',0,0,0,0,0
};

// ==========================================================================
// BOS 描述符 (Binary Object Store)
// 
// USB 2.1 引入的描述符，用于声明设备的扩展能力
// 这里用于声明设备支持 MS OS 2.0 描述符
// ==========================================================================

const uint8_t desc_bos[BOS_TOTAL_LEN] = {
    // ========== BOS 描述符头部 (5 字节) ==========
    0x05,                         // bLength = 5
    0x0F,                         // bDescriptorType = BOS (0x0F)
    USBShort(BOS_TOTAL_LEN),      // wTotalLength = 33
    0x01,                         // bNumDeviceCaps = 1 (一个能力描述符)

    // ========== MS OS 2.0 平台能力描述符 (28 字节) ==========
    // 告诉 Windows 此设备支持 MS OS 2.0 描述符
    0x1C,                         // bLength = 28
    0x10,                         // bDescriptorType = DEVICE CAPABILITY (0x10)
    0x05,                         // bDevCapabilityType = PLATFORM (0x05)
    0x00,                         // bReserved
    // 平台能力 UUID: {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F}
    // 这是 MS OS 2.0 的标准 UUID，Windows 通过此 UUID 识别
    0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06,       // dwWindowsVersion = Windows 8.1+
    USBShort(MS_OS_20_DESC_LEN),  // wMSOSDescriptorSetTotalLength = 162
    MS_VENDOR_CODE,               // bMS_VendorCode = 0x01 (Vendor Request 代码)
    0x00                          // bAltEnumCode = 0 (不使用备用枚举)
};

// ==========================================================================
// TinyUSB 描述符回调函数
// 
// 这些函数由 TinyUSB 协议栈自动调用，你不需要手动调用
// TinyUSB 使用弱符号 (weak symbol)，你实现后会覆盖默认实现
// ==========================================================================

/**
 * @brief BOS 描述符回调
 * 
 * 当主机请求 BOS 描述符时，TinyUSB 调用此函数
 * 仅 USB 2.1+ 设备会收到此请求 (bcdUSB >= 0x0210)
 * 
 * @return BOS 描述符指针
 */
uint8_t const *tud_descriptor_bos_cb(void)
{
    ESP_LOGI(TAG, "tud_descriptor_bos_cb");
    return desc_bos;
}

/**
 * @brief Vendor 控制传输回调
 * 
 * 处理 Vendor 类型的控制请求，主要用于响应 Windows 的 MS OS 2.0 描述符请求
 * 
 * @param rhport USB 端口号 (ESP32-S3 只有一个端口，固定为 0)
 * @param stage  控制传输阶段：SETUP / DATA / ACK
 * @param request 控制请求结构体，包含 bRequest, wValue, wIndex, wLength
 * @return true 表示请求已处理，false 表示请求未处理（会返回 STALL）
 */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // 只在 SETUP 阶段处理请求
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    // 检查是否是 MS OS 2.0 描述符请求
    // Windows 会发送: bRequest = MS_VENDOR_CODE (0x01), wIndex = 7
    if (request->bRequest == MS_VENDOR_CODE && request->wIndex == MS_OS_20_WINDEX) {
        uint16_t len = request->wLength;
        if (len > MS_OS_20_DESC_LEN) {
            len = MS_OS_20_DESC_LEN;
        }
        ESP_LOGI(TAG, "MS OS 2.0 request: wLength=%u, sending %u bytes", (unsigned)request->wLength, (unsigned)len);
        // 发送 MS OS 2.0 描述符给主机
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)desc_ms_os_20, len);
    }

    return false;  // 未处理的请求返回 STALL
}

// ==========================================================================
// 字符串描述符
// 
// 重要：Keil 等调试软件通过产品名中的 "CMSIS-DAP" 字符串识别设备！
// 如果产品名不包含 "CMSIS-DAP"，Keil 将无法识别此设备
// ==========================================================================

/**
 * 字符串描述符数组
 * 
 * 索引 0: 语言 ID (固定为 0x0409 = English US) - 由 esp_tinyusb 自动处理
 * 索引 1: 厂商名称 (对应 desc_device.iManufacturer)
 * 索引 2: 产品名称 (对应 desc_device.iProduct) - 必须包含 "CMSIS-DAP"
 * 索引 3: 序列号 (对应 desc_device.iSerialNumber)
 * 
 * 注意：esp_tinyusb 会自动处理索引 0（语言 ID），所以数组从索引 1 开始
 */
const char *desc_string_arr[] = {
    "",                           // 0: 占位（esp_tinyusb 自动处理语言 ID）
    "XingNian",                   // 1: 厂商名
    "CMSIS-DAP v2",              // 2: 产品名 (必须包含 "CMSIS-DAP"!)
    NULL,                         // 3: 序列号 (动态生成)
};

#define DESC_STRING_COUNT 4

/**
 * @brief 初始化 USB 序列号
 * 
 * 使用 ESP32-S3 的唯一 MAC 地址生成序列号
 * 每个芯片的序列号都不同，适合商用
 * 
 * 必须在 USB 初始化之前调用！
 */
void usb_desc_init_serial(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(serial_number, sizeof(serial_number), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    desc_string_arr[3] = serial_number;
    ESP_LOGI(TAG, "USB Serial: %s", serial_number);
}

/**
 * @brief 获取字符串描述符数组
 */
const char **usb_desc_get_string_arr(void)
{
    return desc_string_arr;
}

/**
 * @brief 获取字符串描述符数量
 */
int usb_desc_get_string_count(void)
{
    return DESC_STRING_COUNT;
}
