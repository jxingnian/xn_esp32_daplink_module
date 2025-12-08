/**
 * @file usb_descriptors.c
 * @brief USB Vendor 类描述符（CMSIS-DAP v2 Bulk 传输）
 * @author 星年
 * @date 2025-12-07
 * 
 * 本文件定义了 CMSIS-DAP v2 设备的 USB 描述符集合，包括：
 * - 设备描述符：告诉主机设备的基本信息（VID/PID/版本等）
 * - 配置描述符：定义设备的接口和端点配置
 * - BOS 描述符：用于声明 Microsoft OS 2.0 扩展描述符支持
 * - MS OS 2.0 描述符：使 Windows 自动加载 WinUSB 驱动（无需 .inf 文件）
 * 
 * CMSIS-DAP v2 使用 USB Vendor 类的 Bulk 传输实现高速数据通信，
 * 相比 v1 的 HID 传输有更高的吞吐量和更低的延迟。
 */

#include <stdint.h>
#include <string.h>
#include "tusb.h"
#include "esp_log.h"

static const char *TAG = "USB_DESC";
static bool s_ms_os_20_dumped = false;

/* ==================== USB 设备描述符 ==================== */
/**
 * USB 设备描述符 - 设备的"身份证"
 * 
 * 这是主机枚举设备时首先读取的描述符，包含设备的基本信息：
 * - VID/PID：用于识别设备制造商和型号
 * - USB 版本：声明支持的 USB 规范版本
 * - 设备类别：0x00 表示在接口级别定义类别（Vendor 类）
 * - 字符串索引：指向厂商名、产品名、序列号的字符串描述符
 */
const tusb_desc_device_t desc_device __attribute__((used)) = {
    .bLength            = sizeof(tusb_desc_device_t),  // 描述符长度（18字节）
    .bDescriptorType    = TUSB_DESC_DEVICE,            // 描述符类型：设备描述符（0x01）
    .bcdUSB             = 0x0201,                      // USB 规范版本：2.01（启用 BOS，用于 MS OS 2.0）
    .bDeviceClass       = 0x00,                        // 设备类：0x00 表示在接口描述符中定义
    .bDeviceSubClass    = 0x00,                        // 设备子类：无
    .bDeviceProtocol    = 0x00,                        // 设备协议：无
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,      // 端点0最大包大小（通常64字节）
    .idVendor           = 0xCAFE,                      // 临时测试厂商ID（避免与 ARM 官方 VID 冲突）
    .idProduct          = 0x4001,                      // 临时测试产品ID
    .bcdDevice          = 0x0200,                      // 设备版本：v2.0
    .iManufacturer      = 0x01,                        // 厂商字符串索引（由回调函数提供）
    .iProduct           = 0x02,                        // 产品字符串索引（由回调函数提供）
    .iSerialNumber      = 0x03,                        // 序列号字符串索引（由回调函数提供）
    .bNumConfigurations = 0x01                         // 配置数量：1个配置
};

/* ==================== 接口和端点编号定义 ==================== */
/**
 * 接口编号枚举
 * 
 * CMSIS-DAP v2 仅需要一个 Vendor 类接口用于 Bulk 传输。
 * ITF_NUM_TOTAL 用于告诉 TinyUSB 配置描述符中有多少个接口。
 */
enum {
    ITF_NUM_VENDOR = 0,  // Vendor 类接口编号（CMSIS-DAP v2）
    ITF_NUM_TOTAL        // 接口总数：1
};

/**
 * 端点地址定义
 * 
 * USB 端点地址格式：[方向位(1bit)][保留(3bit)][端点号(4bit)]
 * - OUT 端点（主机→设备）：最高位为0，用于接收 DAP 命令
 * - IN 端点（设备→主机）：最高位为1（0x80），用于发送 DAP 响应
 * 
 * Full Speed USB 的 Bulk 端点最大包大小为 64 字节。
 */
#define EPNUM_VENDOR_OUT   0x01  // Bulk OUT 端点地址：端点1，方向OUT
#define EPNUM_VENDOR_IN    0x81  // Bulk IN 端点地址：端点1，方向IN（0x80 | 0x01）

/* ==================== USB 配置描述符 ==================== */
/**
 * 配置描述符总长度计算
 * 
 * 配置描述符是一个复合描述符，包含：
 * - 配置描述符头：9 字节
 * - 接口描述符：9 字节
 * - Bulk OUT 端点描述符：7 字节
 * - Bulk IN 端点描述符：7 字节
 * 总计：32 字节
 */
#define CONFIG_TOTAL_LEN  (9 + 9 + 7 + 7)

/**
 * USB 配置描述符 - 定义设备的接口和端点配置
 * 
 * 这个描述符告诉主机设备有哪些接口、每个接口有哪些端点。
 * CMSIS-DAP v2 使用一个 Vendor 类接口，配备一对 Bulk 端点用于双向通信。
 */
const uint8_t desc_fs_configuration[] __attribute__((used)) = {
    /* -------------------- 配置描述符头 -------------------- */
    /**
     * TUD_CONFIG_DESCRIPTOR 宏展开为标准配置描述符（9字节）
     * 参数：
     * - 配置编号：1（第一个也是唯一的配置）
     * - 接口数量：ITF_NUM_TOTAL（1个接口）
     * - 字符串索引：0（无配置字符串描述符）
     * - 总长度：CONFIG_TOTAL_LEN（32字节）
     * - 属性：0x00（总线供电，不支持远程唤醒）
     * - 最大功耗：100mA（以2mA为单位，100表示200mA）
     */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    /* -------------------- CMSIS-DAP v2 接口描述符 -------------------- */
    /**
     * Vendor 类接口描述符（9字节）
     * 
     * 字段说明：
     * - bLength：9（描述符长度）
     * - bDescriptorType：TUSB_DESC_INTERFACE（接口描述符，0x04）
     * - bInterfaceNumber：ITF_NUM_VENDOR（接口编号0）
     * - bAlternateSetting：0（无备用设置）
     * - bNumEndpoints：2（2个端点：Bulk IN + Bulk OUT）
     * - bInterfaceClass：0xFF（Vendor 类，厂商自定义）
     * - bInterfaceSubClass：0x00（无子类）
     * - bInterfaceProtocol：0x00（无协议）
     * - iInterface：0（无接口字符串描述符）
     * 
     * 注意：虽然是 Vendor 类，但通过 MS OS 2.0 描述符声明 WinUSB 兼容 ID，
     * Windows 会自动加载 WinUSB 驱动，无需手动安装 .inf 文件。
     */
    9, TUSB_DESC_INTERFACE, ITF_NUM_VENDOR, 0, 2, 0xFF, 0x00, 0x00, 0,
    
    /* -------------------- Bulk OUT 端点描述符 -------------------- */
    /**
     * Bulk OUT 端点描述符（7字节）- 接收主机发送的 DAP 命令
     * 
     * 字段说明：
     * - bLength：7（描述符长度）
     * - bDescriptorType：TUSB_DESC_ENDPOINT（端点描述符，0x05）
     * - bEndpointAddress：EPNUM_VENDOR_OUT（0x01，OUT方向）
     * - bmAttributes：TUSB_XFER_BULK（Bulk传输，0x02）
     * - wMaxPacketSize：64（Full Speed 最大包大小）
     * - bInterval：0（Bulk端点忽略此字段）
     * 
     * Full Speed USB 限制 Bulk 端点最大包为 64 字节，
     * 高速 USB（480Mbps）可支持 512 字节，但 ESP32-S3 不支持高速。
     */
    7, TUSB_DESC_ENDPOINT, EPNUM_VENDOR_OUT, TUSB_XFER_BULK, U16_TO_U8S_LE(64), 0,
    
    /* -------------------- Bulk IN 端点描述符 -------------------- */
    /**
     * Bulk IN 端点描述符（7字节）- 向主机发送 DAP 响应
     * 
     * 字段说明：
     * - bLength：7（描述符长度）
     * - bDescriptorType：TUSB_DESC_ENDPOINT（端点描述符，0x05）
     * - bEndpointAddress：EPNUM_VENDOR_IN（0x81，IN方向）
     * - bmAttributes：TUSB_XFER_BULK（Bulk传输，0x02）
     * - wMaxPacketSize：64（Full Speed 最大包大小）
     * - bInterval：0（Bulk端点忽略此字段）
     */
    7, TUSB_DESC_ENDPOINT, EPNUM_VENDOR_IN, TUSB_XFER_BULK, U16_TO_U8S_LE(64), 0
};

/* ==================== BOS 描述符（Binary Object Store）==================== */
/**
 * Microsoft OS 2.0 描述符集的长度（simple device 布局）
 *
 * 采用 TinyUSB 讨论 #823（HiFiPhile）给出的 simple device 示例：
 * - Set Header：10 字节
 * - Compatible ID：20 字节
 * - Registry Property：132 字节
 * 总计：162 字节（0xA2）
 */
#define MS_OS_20_DESC_LEN  0xA2

/**
 * BOS 描述符总长度
 * 
 * BOS（Binary Object Store）是 USB 2.0 引入的扩展机制，
 * 用于声明设备支持的高级功能和平台特定描述符。
 * 
 * 组成：
 * - BOS 头：5 字节（TUD_BOS_DESC_LEN）
 * - MS OS 2.0 平台描述符：28 字节（TUD_BOS_MICROSOFT_OS_DESC_LEN）
 * 总计：33 字节
 */
#define BOS_TOTAL_LEN      (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

/**
 * BOS 描述符 - 声明支持 Microsoft OS 2.0 描述符
 * 
 * BOS 描述符告诉主机设备支持哪些扩展功能。
 * 对于 CMSIS-DAP v2，我们声明支持 MS OS 2.0 描述符，
 * 使 Windows 10+ 自动识别设备并加载 WinUSB 驱动。
 * 
 * TUD_BOS_DESCRIPTOR 宏展开为：
 * - bLength：5
 * - bDescriptorType：0x0F（BOS）
 * - wTotalLength：BOS_TOTAL_LEN（33字节）
 * - bNumDeviceCaps：1（1个设备能力描述符）
 * 
 * TUD_BOS_MS_OS_20_DESCRIPTOR 宏展开为 MS OS 2.0 平台描述符（28字节）：
 * - bLength：28
 * - bDescriptorType：0x10（Device Capability）
 * - bDevCapabilityType：0x05（Platform）
 * - bReserved：0
 * - PlatformCapabilityUUID：MS OS 2.0 平台 GUID
 * - dwWindowsVersion：0x06030000（Windows 8.1+）
 * - wMSOSDescriptorSetTotalLength：MS_OS_20_DESC_LEN（178字节）
 * - bMS_VendorCode：1（Vendor Request 代码，用于获取 MS OS 2.0 描述符）
 * - bAltEnumCode：0（不支持备用枚举）
 */
const uint8_t desc_bos[] __attribute__((used)) = {
    // BOS 头（5字节）
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    
    // Microsoft OS 2.0 平台描述符（28字节）
    // 告诉 Windows："我有 MS OS 2.0 描述符，请用 Vendor Request 1 来获取"
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, 1)
};

// Microsoft OS 2.0 描述符集（simple device）
// 完全按照 TinyUSB 讨论 #823 HiFiPhile 示例实现
const uint8_t desc_ms_os_20[] __attribute__((used)) = {
    // Set header: length, type, windows version, total length
    U16_TO_U8S_LE(0x000A),
    U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000),
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
    U16_TO_U8S_LE(0x0014),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

    // MS OS 2.0 Registry property descriptor: length, type
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN-0x0A-0x14),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007),
    U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00,
    'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,
    'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00,
    'D', 0x00, 's', 0x00, 0x00, 0x00,
    U16_TO_U8S_LE(0x0050), // wPropertyDataLength
    // bPropertyData: {70394F16-EDAF-47D5-92C8-7CB51107A235}
    '{', 0x00, '7', 0x00, '0', 0x00, '3', 0x00, '9', 0x00, '4', 0x00,
    'F', 0x00, '1', 0x00, '6', 0x00, '-', 0x00,
    'E', 0x00, 'D', 0x00, 'A', 0x00, 'F', 0x00, '-', 0x00,
    '4', 0x00, '7', 0x00, 'D', 0x00, '5', 0x00, '-', 0x00,
    '9', 0x00, '2', 0x00, 'C', 0x00, '8', 0x00, '-', 0x00,
    '7', 0x00, 'C', 0x00, 'B', 0x00, '5', 0x00, '1', 0x00, '1', 0x00,
    '0', 0x00, '7', 0x00, 'A', 0x00, '2', 0x00, '3', 0x00, '5', 0x00,
    '}', 0x00, 0x00, 0x00, 0x00, 0x00
};

/**
 * @brief BOS 描述符回调函数
 *
 * esp_tinyusb 组件没有提供此回调，需要我们自己实现。
 * 当主机请求 BOS 描述符时，TinyUSB 会调用此函数。
 *
 * @return 指向 BOS 描述符的指针
 */
uint8_t const* tud_descriptor_bos_cb(void)
{
    ESP_LOGI(TAG, "tud_descriptor_bos_cb called");
    return desc_bos;
}

/**
 * @brief Vendor 类控制传输回调函数
 * 
 * esp_tinyusb 组件没有提供此回调，需要我们自己实现。
 * 处理 Vendor 类的控制请求，主要用于返回 MS OS 2.0 描述符。
 * 
 * @param rhport USB 端口号
 * @param stage 控制传输阶段 (SETUP/DATA/ACK)
 * @param request 控制请求结构体
 * @return true 请求处理成功，false 请求不支持（STALL）
 */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // 只处理 SETUP 阶段
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    ESP_LOGI(TAG,
             "vendor ctrl: bmReq=0x%02X bReq=%u wIndex=%u wLength=%u",
             request->bmRequestType,
             request->bRequest,
             request->wIndex,
             request->wLength);

    // 检查是否是 MS OS 2.0 描述符请求
    // bRequest = 1 (BOS 描述符中定义的 Vendor Code)
    // wIndex = 7 (MS OS 2.0 描述符集请求)
    if (request->bRequest == 1 && request->wIndex == 7) {
        if (!s_ms_os_20_dumped) {
            s_ms_os_20_dumped = true;
            ESP_LOGI(TAG, "MS OS 2.0: sizeof=%u, MS_OS_20_DESC_LEN=%u",
                     (unsigned)sizeof(desc_ms_os_20), (unsigned)MS_OS_20_DESC_LEN);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, desc_ms_os_20, MS_OS_20_DESC_LEN, ESP_LOG_INFO);
        }
        ESP_LOGI(TAG, "sending MS OS 2.0 descriptor, len=%u", (unsigned)MS_OS_20_DESC_LEN);
        // 返回 MS OS 2.0 描述符
        return tud_control_xfer(rhport, request, (void*)(uintptr_t)desc_ms_os_20, MS_OS_20_DESC_LEN);
    }

    // 不支持的请求
    return false;
}
