#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_types.h"
#include "esp_event.h"
#include "esp_err.h"
#include "driver/uart.h"

// TODO: make this part of sdkconfig
#define ESP_GPS_UART_RXD 18

/**
* @brief Declare of ESP GPS Event base
*
*/
ESP_EVENT_DECLARE_BASE(ESP_GPS_EVENT);

/**
 * @brief ESP GPS Event IDs
 *
 */
typedef enum {
    ESP_GPS_NMEA_UPDATE, /*!< NMEA GPS update */
    ESP_GPS_UNKNOWN /*!< Unknown GPS data */
} esp_gps_event_id_t;

typedef esp_err_t (*esp_gps_parser_t)(void *esp_gps, size_t len);

/**
* @brief ESP GPS library runtime structure
*
*/
typedef struct {
//    nmea_s nmea_data;                              /*!< Data parsed by nmea_parse */
    uart_port_t uart_port;                         /*!< Uart port number */
    uint8_t *buffer;                               /*!< Runtime buffer */
    esp_event_loop_handle_t event_loop_hdl;        /*!< Event loop handle */
    TaskHandle_t tsk_hdl;                          /*!< GPS Data Parser task handle */
    QueueHandle_t event_queue;                     /*!< UART event queue handle */
    esp_gps_parser_t gps_parser;
} esp_gps_t;

/**
* @brief Configuration of ESP GPS
*
*/
typedef struct {
    struct {
        uart_port_t uart_port;        /*!< UART port number */
        uint32_t rx_pin;              /*!< UART Rx Pin number */
        uint32_t baud_rate;           /*!< UART baud rate */
        uart_word_length_t data_bits; /*!< UART data bits length */
        uart_parity_t parity;         /*!< UART parity */
        uart_stop_bits_t stop_bits;   /*!< UART stop bits length */
        uint32_t event_queue_size;    /*!< UART event queue size */
    } uart;                           /*!< UART specific configuration */
    esp_gps_parser_t gps_parser;
} esp_gps_config_t;

/**
 * @brief ESP GPS Handle
 *
 */
typedef void *esp_gps_handle_t;

esp_gps_handle_t esp_gps_init(const esp_gps_config_t *config);

/**
 * @brief Add user defined handler for ESP GPS
 *
 * @param gps_hdl handle of ESP GPS
 * @param event_handler user defined event handler
 * @param handler_args handler specific arguments
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory for the handler
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t esp_gps_add_handler(esp_gps_handle_t gps_hdl, esp_event_handler_t event_handler, void *handler_args);

/**
 * @brief Remove user defined handler for ESP GPS
 *
 * @param gps_hdl handle of ESP GPS
 * @param event_handler user defined event handler
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t esp_gps_remove_handler(esp_gps_handle_t gps_hdl, esp_event_handler_t event_handler);

/**
 * @brief Deinit ESP GPS
 *
 * @param gps_hdl handle of ESP GPS
 * @return esp_err_t ESP_OK on success,ESP_FAIL on error
 */
esp_err_t esp_gps_deinit(esp_gps_handle_t gps_hdl);

#ifdef __cplusplus
}
#endif