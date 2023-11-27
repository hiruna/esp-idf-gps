#include <stdio.h>
#include "esp_gps.h"
#include "esp_gps.h"
#include "nmea_parser.h"
#include "esp_log.h"

#define TAG "gps_reader"

static void
gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    char buf[255];
    nmea_gpgga_s *gpgga;
    nmea_gpgll_s *gpgll;
    nmea_gpgsa_s *gpgsa;
    nmea_gpgsv_s *gpgsv;
    nmea_gprmc_s *gprmc;
    nmea_gptxt_s *gptxt;
    nmea_gpvtg_s *gpvtg;

    nmea_update_t *nmea_update = NULL;
    switch (event_id) {
        case ESP_GPS_NMEA_UPDATE:
            nmea_update = (nmea_update_t *) event_data;
            switch (nmea_update->type) {
                case NMEA_GPGGA:
                    ESP_LOGI(TAG, "GPGGA sentence\n");
                    gpgga = nmea_update->data.gpgga;
                    ESP_LOGI(TAG, "Number of satellites: %d\n", gpgga->n_satellites);
                    ESP_LOGI(TAG, "Altitude: %f %c\n", gpgga->altitude, gpgga->altitude_unit);
                    break;
                case NMEA_GPGLL:
                    ESP_LOGI(TAG, "GPGLL sentence\n");
                    gpgll = nmea_update->data.gpgll;
                    ESP_LOGI(TAG, "Longitude:\n");
                    ESP_LOGI(TAG, "  Degrees: %d\n", gpgll->longitude.degrees);
                    ESP_LOGI(TAG, "  Minutes: %f\n", gpgll->longitude.minutes);
                    ESP_LOGI(TAG, "  Cardinal: %c\n", (char) gpgll->longitude.cardinal);
                    ESP_LOGI(TAG, "Latitude:\n");
                    ESP_LOGI(TAG, "  Degrees: %d\n", gpgll->latitude.degrees);
                    ESP_LOGI(TAG, "  Minutes: %f\n", gpgll->latitude.minutes);
                    ESP_LOGI(TAG, "  Cardinal: %c\n", (char) gpgll->latitude.cardinal);
                    strftime(buf, sizeof(buf), "%H:%M:%S", &gpgll->time);
                    ESP_LOGI(TAG, "Time: %s\n", buf);
                    break;
                case NMEA_GPGSA:
                    gpgsa = nmea_update->data.gpgsa;

                    ESP_LOGI(TAG, "GPGSA Sentence:\n");
                    ESP_LOGI(TAG, "  Mode: %c\n", gpgsa->mode);
                    ESP_LOGI(TAG, "  Fix:  %d\n", gpgsa->fixtype);
                    ESP_LOGI(TAG, "  PDOP: %.2lf\n", gpgsa->pdop);
                    ESP_LOGI(TAG, "  HDOP: %.2lf\n", gpgsa->hdop);
                    ESP_LOGI(TAG, "  VDOP: %.2lf\n", gpgsa->vdop);
                    break;
                case NMEA_GPGSV:
                    gpgsv = nmea_update->data.gpgsv;

                    ESP_LOGI(TAG, "GPGSV Sentence:\n");
                    ESP_LOGI(TAG, "  Num: %d\n", gpgsv->sentences);
                    ESP_LOGI(TAG, "  ID:  %d\n", gpgsv->sentence_number);
                    ESP_LOGI(TAG, "  SV:  %d\n", gpgsv->satellites);
                    ESP_LOGI(TAG, "  #1:  %d %d %d %d\n", gpgsv->sat[0].prn, gpgsv->sat[0].elevation,
                             gpgsv->sat[0].azimuth, gpgsv->sat[0].snr);
                    ESP_LOGI(TAG, "  #2:  %d %d %d %d\n", gpgsv->sat[1].prn, gpgsv->sat[1].elevation,
                             gpgsv->sat[1].azimuth, gpgsv->sat[1].snr);
                    ESP_LOGI(TAG, "  #3:  %d %d %d %d\n", gpgsv->sat[2].prn, gpgsv->sat[2].elevation,
                             gpgsv->sat[2].azimuth, gpgsv->sat[2].snr);
                    ESP_LOGI(TAG, "  #4:  %d %d %d %d\n", gpgsv->sat[3].prn, gpgsv->sat[3].elevation,
                             gpgsv->sat[3].azimuth, gpgsv->sat[3].snr);
                    break;
                case NMEA_GPRMC:
                    ESP_LOGI(TAG, "GPRMC sentence\n");
                    gprmc = nmea_update->data.gprmc;
                    ESP_LOGI(TAG, "Longitude:\n");
                    ESP_LOGI(TAG, "  Degrees: %d\n", gprmc->longitude.degrees);
                    ESP_LOGI(TAG, "  Minutes: %f\n", gprmc->longitude.minutes);
                    ESP_LOGI(TAG, "  Cardinal: %c\n", (char) gprmc->longitude.cardinal);
                    ESP_LOGI(TAG, "Latitude:\n");
                    ESP_LOGI(TAG, "  Degrees: %d\n", gprmc->latitude.degrees);
                    ESP_LOGI(TAG, "  Minutes: %f\n", gprmc->latitude.minutes);
                    ESP_LOGI(TAG, "  Cardinal: %c\n", (char) gprmc->latitude.cardinal);
                    strftime(buf, sizeof(buf), "%d %b %T %Y", &gprmc->date_time);
                    ESP_LOGI(TAG, "Date & Time: %s\n", buf);
                    ESP_LOGI(TAG, "Speed, in Knots: %f:\n", gprmc->gndspd_knots);
                    ESP_LOGI(TAG, "Track, in degrees: %f\n", gprmc->track_deg);
                    ESP_LOGI(TAG, "Magnetic Variation:\n");
                    ESP_LOGI(TAG, "  Degrees: %f\n", gprmc->magvar_deg);
                    ESP_LOGI(TAG, "  Cardinal: %c\n", (char) gprmc->magvar_cardinal);
                    double adjusted_course = gprmc->track_deg;
                    if (NMEA_CARDINAL_DIR_EAST == gprmc->magvar_cardinal) {
                        adjusted_course -= gprmc->magvar_deg;
                    } else if (NMEA_CARDINAL_DIR_WEST == gprmc->magvar_cardinal) {
                        adjusted_course += gprmc->magvar_deg;
                    } else {
                        ESP_LOGI(TAG, "Invalid Magnetic Variation Direction!!\n");
                    }

                    ESP_LOGI(TAG, "Adjusted Track (heading): %f\n", adjusted_course);

                    break;
                case NMEA_GPTXT:
                    gptxt = nmea_update->data.gptxt;

                    ESP_LOGI(TAG, "GPTXT Sentence:\n");
                    ESP_LOGI(TAG, "  ID: %d %d %d\n", gptxt->id_00, gptxt->id_01, gptxt->id_02);
                    ESP_LOGI(TAG, "  %s\n", gptxt->text);
                    break;
                case NMEA_GPVTG:
                    gpvtg = nmea_update->data.gpvtg;

                    ESP_LOGI(TAG, "GPVTG Sentence:\n");
                    ESP_LOGI(TAG, "  Track [deg]:   %.2lf\n", gpvtg->track_deg);
                    ESP_LOGI(TAG, "  Speed [kmph]:  %.2lf\n", gpvtg->gndspd_kmph);
                    ESP_LOGI(TAG, "  Speed [knots]: %.2lf\n", gpvtg->gndspd_knots);
                    break;
                case NMEA_UNKNOWN:
                    ESP_LOGI(TAG, "NMEA_UNKNOWN");
                    break;
            }
            break;
        case ESP_GPS_UNKNOWN:
            /* print unknown statements */
            ESP_LOGW(TAG, "Unknown statement:%s", (char *) event_data);
            break;
        default:
            break;
    }

}

void app_main() {
    esp_gps_config_t esp_gps_config = {
            .uart = {
                    .uart_port = UART_NUM_1,
                    .rx_pin = ESP_GPS_UART_RXD,
                    .baud_rate = 9600,
                    .data_bits = UART_DATA_8_BITS,
                    .parity = UART_PARITY_DISABLE,
                    .stop_bits = UART_STOP_BITS_1,
                    .event_queue_size = 2048
            },
            .gps_parser = gps_parse_nmea
    };
    esp_gps_handle_t gps_hdl = esp_gps_init(&esp_gps_config);
    esp_gps_add_handler(gps_hdl, gps_event_handler, NULL);

    vTaskDelay(10000 / portTICK_PERIOD_MS);

    /* unregister event handler */
    esp_gps_remove_handler(gps_hdl, gps_event_handler);

    /* deinit NMEA parser library */
    esp_gps_deinit(gps_hdl);
}