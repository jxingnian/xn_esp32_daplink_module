/**
 * @file dap_configuration.h
 * @brief DAP configuration for ESP32-S3 USB direct connection
 */

#ifndef __DAP_CONFIGURATION_H__
#define __DAP_CONFIGURATION_H__

// Use WinUSB mode (DAP v2)
#define USE_WINUSB 1

// Enable SPI single-wire mode (no need to physically connect MOSI and MISO)
#define USE_SPI_SIO 1

// USB endpoint size for Full-Speed USB
#define USB_ENDPOINT_SIZE 64U

// DAP packet size for USB Full-Speed WinUSB
#define DAP_PACKET_SIZE 64U

// Disable force software reset
#define USE_FORCE_SYSRESETREQ_AFTER_FLASH 0

#endif // __DAP_CONFIGURATION_H__
