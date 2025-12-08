#include "tusb.h"
#include "esp_log.h"

static const char *TAG = "USB_DESC";

// --------------------------------------------------------------------------
// Device & configuration descriptors (TinyUSB format)
// --------------------------------------------------------------------------

#define USB_VID   0x303A
#define USB_PID   0x4001

enum {
    ITF_NUM_VENDOR = 0,
    ITF_NUM_TOTAL
};

#define EPNUM_VENDOR_OUT   0x01
#define EPNUM_VENDOR_IN    0x81

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

// Device descriptor
const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0210,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0200,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// Full-speed configuration descriptor
const uint8_t desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Vendor interface: 1 OUT + 1 IN bulk endpoint
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT,
                          EPNUM_VENDOR_IN, 64),
};

// --------------------------------------------------------------------------
// MS OS 2.0 & BOS descriptors (ported from wireless-esp8266-dap)
// --------------------------------------------------------------------------

#define MS_OS_20_DESC_LEN   0xA2
#define BOS_TOTAL_LEN       0x21
#define MS_VENDOR_CODE      0x01
#define MS_OS_20_WINDEX     7

#define USBShort(v)   ((uint8_t)((v) & 0xFF)), ((uint8_t)(((v) >> 8) & 0xFF))

// Microsoft OS 2.0 descriptor set header + Compatible ID + Registry Property
const uint8_t desc_ms_os_20[MS_OS_20_DESC_LEN] = {
    // Set header (Table 10)
    0x0A, 0x00,                  // wLength
    0x00, 0x00,                  // MS_OS_20_SET_HEADER_DESCRIPTOR
    0x00, 0x00, 0x03, 0x06,      // dwWindowsVersion: 0x06030000 (Windows 8.1)
    USBShort(MS_OS_20_DESC_LEN), // wTotalLength

    // Compatible ID descriptor (Table 13)
    0x14, 0x00,                  // wLength
    USBShort(0x0003),            // wDescriptorType = MS_OS_20_FEATURE_COMPATIBLE_ID
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,  // compatibleID = "WINUSB\0\0"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // subCompatibleID

    // Registry property descriptor (Table 14)
    0x84, 0x00,                  // wLength
    USBShort(0x0004),            // wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY
    0x07, 0x00,                  // wPropertyDataType: REG_MULTI_SZ
    0x2A, 0x00,                  // wPropertyNameLength
    // PropertyName: "DeviceInterfaceGUIDs\0" in UTF-16LE
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,'r',0,
    'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,
    0x50, 0x00,                  // wPropertyDataLength (80 bytes)
    // bPropertyData: "{CDB3B5AD-293B-4663-AA36-1AAE46463776}\0" UTF-16LE
    '{',0,'C',0,'D',0,'B',0,'3',0,'B',0,'5',0,'A',0,'D',0,'-',0,
    '2',0,'9',0,'3',0,'B',0,'-',0,'4',0,'6',0,'6',0,'3',0,'-',0,
    'A',0,'A',0,'3',0,'6',0,'-',0,'1',0,'A',0,'A',0,'E',0,'4',0,
    '6',0,'4',0,'6',0,'3',0,'7',0,'7',0,'6',0,'}',0,0,0,0,0
};

// BOS descriptor with MS OS 2.0 Platform Capability
const uint8_t desc_bos[BOS_TOTAL_LEN] = {
    // BOS descriptor header
    0x05,                         // bLength
    0x0F,                         // bDescriptorType = BOS
    USBShort(BOS_TOTAL_LEN),      // wTotalLength
    0x01,                         // bNumDeviceCaps

    // Microsoft OS 2.0 Platform Capability Descriptor (Table 4)
    0x1C,                         // bLength
    0x10,                         // bDescriptorType = DEVICE CAPABILITY
    0x05,                         // bDevCapabilityType = PLATFORM
    0x00,                         // bReserved
    // MS_OS_20_Platform_Capability_ID
    0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06,       // dwWindowsVersion
    USBShort(MS_OS_20_DESC_LEN),  // wMSOSDescriptorSetTotalLength
    MS_VENDOR_CODE,               // bMS_VendorCode
    0x00                          // bAltEnumCode
};

// --------------------------------------------------------------------------
// TinyUSB descriptor callbacks
// --------------------------------------------------------------------------

uint8_t const *tud_descriptor_bos_cb(void)
{
    ESP_LOGI(TAG, "tud_descriptor_bos_cb");
    return desc_bos;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    if (request->bRequest == MS_VENDOR_CODE && request->wIndex == MS_OS_20_WINDEX) {
        uint16_t len = request->wLength;
        if (len > MS_OS_20_DESC_LEN) {
            len = MS_OS_20_DESC_LEN;
        }
        ESP_LOGI(TAG, "MS OS 2.0 request: wLength=%u, sending %u bytes", (unsigned)request->wLength, (unsigned)len);
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)desc_ms_os_20, len);
    }

    return false;
}
