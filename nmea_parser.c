#include "nmea_parser.h"
#include "esp_log.h"

static const char *NMEA_PARSER_TAG = "nmea_parser";

/**
 * @brief Using libnmea, parse/decode the input data
 *
 * @param gps esp_gps type obj
 * @param len number of read bytes
 * @return ESP_FAIL if nmea_parse has errors
 * @return ESP_ERR_INVALID_RESPONSE if parsed nmea data was not of expected tyoe
 * @return ESP_OK if successful
 */
esp_err_t gps_parse_nmea(void *gps, size_t len) {
    const esp_gps_t *esp_gps = (esp_gps_t *) gps;
    const char *buff = (char *) esp_gps->buffer;

    nmea_s *data = nmea_parse((char *) buff, len, 0);
    if (data != NULL) {
        if (data->errors > 0) {
            ESP_LOGW(NMEA_PARSER_TAG, "nmea_parse data contains errors!");
            return ESP_FAIL;
        }
        // esp_gps->nmea_data = *data;

        nmea_update_t nmea_update = {
                .type = data->type,
        };
        nmea_update.type = data->type;
        switch (data->type) {
            case NMEA_GPGGA:
                nmea_update.data.gpgga = (nmea_gpgga_s *) data;
                break;
            case NMEA_GPGLL:
                nmea_update.data.gpgll = (nmea_gpgll_s *) data;
                break;
            case NMEA_GPGSA:
                nmea_update.data.gpgsa = (nmea_gpgsa_s *) data;
                break;
            case NMEA_GPGSV:
                nmea_update.data.gpgsv = (nmea_gpgsv_s *) data;
                break;
            case NMEA_GPRMC:
                nmea_update.data.gprmc = (nmea_gprmc_s *) data;
                break;
            case NMEA_GPTXT:
                nmea_update.data.gptxt = (nmea_gptxt_s *) data;
                break;
            case NMEA_GPVTG:
                nmea_update.data.gpvtg = (nmea_gpvtg_s *) data;
                break;
            case NMEA_UNKNOWN:
                ESP_LOGW(NMEA_PARSER_TAG, "nmea_parse returned NMEA_UNKNOWN");
                break;
            default:
                esp_event_post_to(esp_gps->event_loop_hdl, ESP_GPS_EVENT, ESP_GPS_UNKNOWN,
                                  esp_gps->buffer, len, 100 / portTICK_PERIOD_MS);
                nmea_free(data);
                return ESP_ERR_INVALID_RESPONSE;
        }

        esp_event_post_to(esp_gps->event_loop_hdl, ESP_GPS_EVENT, ESP_GPS_NMEA_UPDATE,
                          &nmea_update, sizeof(nmea_update), 100 / portTICK_PERIOD_MS);

        nmea_free(data);
    } else {
        ESP_LOGI(NMEA_PARSER_TAG, "nmea_parse data is null");
    }


    return ESP_OK;
}

/**
 * @brief helper: parse nmea_position (lat/long)
 * @param pos nmea_position type object
 * @return double Latitude or Longitude value (unit: degree.minutes)
 */
double parse_nmea_position(nmea_position pos) {
    return pos.degrees + pos.minutes / 60.0f;
}
