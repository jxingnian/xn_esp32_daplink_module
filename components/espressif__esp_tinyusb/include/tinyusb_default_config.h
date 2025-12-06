/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include "tinyusb.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GET_CONFIG_MACRO(dummy, arg1, arg2, arg3, name, ...)    name

/**
 * @brief Default TinyUSB Driver configuration structure initializer
 *
 * Default port:
 * - ESP32P4:       USB OTG 2.0 (High-speed)
 * - ESP32S2/S3:    USB OTG 1.1 (Full-speed)
 *
 * Default size:
 * - 4096 bytes
 * Default priority:
 * - 5
 *
 * Default task affinity:
 * - Multicore:     CPU1
 * - Unicore:       CPU0
 *
 */

#define TINYUSB_DEFAULT_CONFIG(...)              GET_CONFIG_MACRO(, ##__VA_ARGS__, \
                                                                    TINYUSB_CONFIG_INVALID,    \
                                                                    TINYUSB_CONFIG_EVENT_ARG,  \
                                                                    TINYUSB_CONFIG_EVENT,      \
                                                                    TINYUSB_CONFIG_NO_ARG      \
                                                                )(__VA_ARGS__)

#define TINYUSB_CONFIG_INVALID(...)              static_asser