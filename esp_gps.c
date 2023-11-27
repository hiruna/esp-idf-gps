#include "esp_gps.h"
#include "esp_log.h"

// TODO: make these part of sdkconfig
#define ESP_GPS_RING_BUFFER_SIZE 2048 // 2048
#define ESP_GPS_TASK_STACK_SIZE 3072 // 4096
#define ESP_GPS_TASK_PRIORITY 2 // 24
#define ESP_GPS_RUNTIME_BUFFER_SIZE (ESP_GPS_RING_BUFFER_SIZE / 2)
#define ESP_GPS_EVENT_LOOP_QUEUE_SIZE (16)

static const char *ESP_GPS_TAG = "esp_gps";

/**
 * @brief Define of ESP GPS Event base
 *
 */
ESP_EVENT_DEFINE_BASE(ESP_GPS_EVENT);

/**
 * @brief Handle when a pattern has been detected by uart
 *
 * @param esp_gps esp_gps_t type object
 */

static void esp_gps_handle_uart_pattern(esp_gps_t *esp_gps) {
    int pos = uart_pattern_pop_pos(esp_gps->uart_port);
    if (pos != -1) {
        /* read one line(include '\n') */
        int read_len = uart_read_bytes(esp_gps->uart_port, esp_gps->buffer, pos + 1, 100 / portTICK_PERIOD_MS);
        /* make sure the line is a standard string */
        esp_gps->buffer[read_len] = '\0';

        /* Send new line to handle */
        if (esp_gps->gps_parser(esp_gps, read_len) != ESP_OK) {
            ESP_LOGW(ESP_GPS_TAG, "gps_parser failed!");
        }
    } else {
        ESP_LOGW(ESP_GPS_TAG, "Pattern Queue Size too small");
        uart_flush_input(esp_gps->uart_port);
    }
}


/**
 * @brief ESP GPS Task Entry
 *
 * @param arg argument
 */
static void esp_gps_task_entry(void *arg) {
    esp_gps_t *esp_gps = (esp_gps_t *) arg;
    uart_event_t event;
    while (1) {
        if (xQueueReceive(esp_gps->event_queue, &event, pdMS_TO_TICKS(200))) {
            switch (event.type) {
                case UART_DATA:
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGW(ESP_GPS_TAG, "HW FIFO Overflow");
                    uart_flush(esp_gps->uart_port);
                    xQueueReset(esp_gps->event_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGW(ESP_GPS_TAG, "Ring Buffer Full");
                    uart_flush(esp_gps->uart_port);
                    xQueueReset(esp_gps->event_queue);
                    break;
                case UART_BREAK:
                    ESP_LOGW(ESP_GPS_TAG, "Rx Break");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGE(ESP_GPS_TAG, "Parity Error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGE(ESP_GPS_TAG, "Frame Error");
                    break;
                case UART_PATTERN_DET:
                    esp_gps_handle_uart_pattern(esp_gps);
                    break;
                default:
                    ESP_LOGW(ESP_GPS_TAG, "unknown uart event type: %d", event.type);
                    break;
            }
        }
        /* Drive the event loop */
        esp_event_loop_run(esp_gps->event_loop_hdl, pdMS_TO_TICKS(50));
    }
    vTaskDelete(NULL);
}

/**
 * @brief Init ESP GPS
 *
 * @param config Configuration of ESP GPS
 * @return esp_gps_handle_t handle of esp_gps
 */
esp_gps_handle_t esp_gps_init(const esp_gps_config_t *config) {
    esp_gps_t *esp_gps = calloc(1, sizeof(esp_gps_t));
    if (!esp_gps) {
        ESP_LOGE(ESP_GPS_TAG, "calloc memory for esp_gps failed");
        goto err_gps;
    }

    esp_gps->gps_parser = config->gps_parser;

    esp_gps->buffer = calloc(1, ESP_GPS_RUNTIME_BUFFER_SIZE);
    if (!esp_gps->buffer) {
        ESP_LOGE(ESP_GPS_TAG, "calloc memory for runtime buffer failed");
        goto err_buffer;
    }

    /* Set attributes */
    esp_gps->uart_port = config->uart.uart_port;
    /* Install UART driver */
    uart_config_t uart_config = {
            .baud_rate = config->uart.baud_rate,
            .data_bits = config->uart.data_bits,
            .parity = config->uart.parity,
            .stop_bits = config->uart.stop_bits,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
    };
    if (uart_driver_install(esp_gps->uart_port, ESP_GPS_RING_BUFFER_SIZE, 0,
                            config->uart.event_queue_size, &esp_gps->event_queue, 0) != ESP_OK) {
        ESP_LOGE(ESP_GPS_TAG, "install uart driver failed");
        goto err_uart_install;
    }
    if (uart_param_config(esp_gps->uart_port, &uart_config) != ESP_OK) {
        ESP_LOGE(ESP_GPS_TAG, "config uart parameter failed");
        goto err_uart_config;
    }
    if (uart_set_pin(esp_gps->uart_port, UART_PIN_NO_CHANGE, config->uart.rx_pin,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(ESP_GPS_TAG, "config uart gpio failed");
        goto err_uart_config;
    }
    /* Set pattern interrupt, used to detect the end of a line */
    uart_enable_pattern_det_baud_intr(esp_gps->uart_port, '\n', 1, 9, 0, 0);
    /* Set pattern queue size */
    uart_pattern_queue_reset(esp_gps->uart_port, config->uart.event_queue_size);
    uart_flush(esp_gps->uart_port);
    /* Create Event loop */
    esp_event_loop_args_t loop_args = {
            .queue_size = ESP_GPS_EVENT_LOOP_QUEUE_SIZE,
            .task_name = NULL
    };
    if (esp_event_loop_create(&loop_args, &esp_gps->event_loop_hdl) != ESP_OK) {
        ESP_LOGE(ESP_GPS_TAG, "create event loop failed");
        goto err_eloop;
    }
    /* Create ESP GPS task */
    BaseType_t err = xTaskCreate(
            esp_gps_task_entry,
            "esp_gps",
            ESP_GPS_TASK_STACK_SIZE,
            esp_gps,
            ESP_GPS_TASK_PRIORITY,
            &esp_gps->tsk_hdl);
    if (err != pdTRUE) {
        ESP_LOGE(ESP_GPS_TAG, "create ESP GPS task failed");
        goto err_task_create;
    }
    ESP_LOGI(ESP_GPS_TAG, "ESP GPS init OK");
    return esp_gps;
    /*Error Handling*/
    err_task_create:
    esp_event_loop_delete(esp_gps->event_loop_hdl);
    err_eloop:
    err_uart_install:
    uart_driver_delete(esp_gps->uart_port);
    err_uart_config:
    err_buffer:
    free(esp_gps->buffer);
    err_gps:
    free(esp_gps);
    return NULL;
}

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
esp_err_t esp_gps_add_handler(esp_gps_handle_t gps_hdl, esp_event_handler_t event_handler, void *handler_args) {
    esp_gps_t *esp_gps = (esp_gps_t *) gps_hdl;
    return esp_event_handler_register_with(esp_gps->event_loop_hdl, ESP_GPS_EVENT, ESP_EVENT_ANY_ID,
                                           event_handler, handler_args);
}

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
esp_err_t esp_gps_remove_handler(esp_gps_handle_t gps_hdl, esp_event_handler_t event_handler) {
    esp_gps_t *esp_gps = (esp_gps_t *) gps_hdl;
    return esp_event_handler_unregister_with(esp_gps->event_loop_hdl, ESP_GPS_EVENT, ESP_EVENT_ANY_ID, event_handler);
}

/**
 * @brief Deinit ESP GPS
 *
 * @param gps_hdl handle of ESP GPS
 * @return esp_err_t ESP_OK on success,ESP_FAIL on error
 */
esp_err_t esp_gps_deinit(esp_gps_handle_t gps_hdl) {
    esp_gps_t *esp_gps = (esp_gps_t *) gps_hdl;
    vTaskDelete(esp_gps->tsk_hdl);
    esp_event_loop_delete(esp_gps->event_loop_hdl);
    esp_err_t err = uart_driver_delete(esp_gps->uart_port);
    free(esp_gps->buffer);
    free(esp_gps);
    return err;
}