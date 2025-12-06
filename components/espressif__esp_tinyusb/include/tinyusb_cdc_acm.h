/*
 * SPDX-FileCopyrightText: 2020-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "class/cdc/cdc.h"

#if (CONFIG_TINYUSB_CDC_ENABLED != 1)
#error "TinyUSB CDC driver must be enabled in menuconfig"
#endif


/**
 * @brief CDC ports available to setup
 */
typedef enum {
    TINYUSB_CDC_ACM_0 = 0x0,
    TINYUSB_CDC_ACM_1,
    TINYUSB_CDC_ACM_MAX
} tinyusb_cdcacm_itf_t;

/*************************************************************************/
/*                      Callbacks and events                             */
/*************************************************************************/

/**
 * @brief Data provided to the input of the `callback_rx_wanted_char` callback
 */
typedef struct {
    char wanted_char; /*!< Wanted character */
} cdcacm_event_rx_wanted_char_data_t;

/**
 * @brief Data provided to the input of the `callback_line_state_changed` callback
 */
typedef struct {
    bool dtr; /*!< Data Terminal Ready (DTR) line state */
    bool rts; /*!< Request To Send (RTS) line state */
} cdcacm_event_line_state_changed_data_t;

/**
 * @brief Data provided to the input of the `line_coding_changed` callback
 */
typedef struct {
    cdc_line_coding_t const *p_line_coding; /*!< New line coding value */
} cdcacm_event_line_coding_changed_data_t;

/**
 * @brief Types of CDC ACM events
 */
typedef enum {
    CDC_EVENT_RX,
    CDC_EVENT_RX_WANTED_CHAR,
    CDC_EVENT_LINE_STATE_CHANGED,
    CDC_EVENT_LINE_CODING_CHANGED
} cdcacm_event_type_t;

/**
 * @brief Describes an event passing to the input of a callbacks
 */
typedef struct {
    cdcacm_event_type_t type; /*!< Event type */
    union {
        cdcacm_event_rx_wanted_char_data_t rx_wanted_char_data; /*!< Data input of the `callback_rx_wanted_char` callback */
        cdcacm_event_line_state_changed_data_t line_state_changed_data; /*!< Data input of the `callback_line_state_changed` callback */
        cdcacm_event_line_coding_changed_data_t line_coding_changed_data; /*!< Data input of the `line_coding_changed` callback */
    };
} cdcacm_event_t;

/**
 * @brief CDC-ACM callback type
 */
typedef void(*tusb_cdcacm_callback_t)(int itf, cdcacm_event_t *event);

/**
 * @brief Configuration structure for CDC-ACM
 */
typedef struct {
    tinyusb_cdcacm_itf_t cdc_port;  /*!< CDC port */
    tusb_cdcacm_callback_t callback_rx;  /*!< Pointer to the function with the `tusb_cdcacm_callback_t` type that will be handled as a callback */
    tusb_cdcacm_callback_t callback_rx_wanted_char; /*!< Pointer to the function with the `tusb_cdcacm_callback_t` type that will be handled as a callback 