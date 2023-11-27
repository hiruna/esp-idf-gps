#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_gps.h"
#include "gpgll.h"
#include "gpgga.h"
#include "gprmc.h"
#include "gpgsa.h"
#include "gpvtg.h"
#include "gptxt.h"
#include "gpgsv.h"
    
typedef struct {
    nmea_gpgga_s *gpgga;
    nmea_gpgll_s *gpgll;
    nmea_gpgsa_s *gpgsa;
    nmea_gpgsv_s *gpgsv;
    nmea_gprmc_s *gprmc;
    nmea_gptxt_s *gptxt;
    nmea_gpvtg_s *gpvtg;
} nmea_data_t;

typedef struct {
    nmea_data_t data;
    nmea_t type;
} nmea_update_t;

/**
 * @brief Using libnmea, parse/decode the input data
 *
 * @param gps esp_gps type obj
 * @param len number of read bytes
 * @return ESP_FAIL if nmea_parse has errors
 * @return ESP_ERR_INVALID_RESPONSE if parsed nmea data was not of expected tyoe
 * @return ESP_OK if successful
 */
esp_err_t gps_parse_nmea(void *e, size_t len);

/**
 * @brief helper: parse nmea_position (lat/long)
 * @param pos nmea_position type object
 * @return double Latitude or Longitude value (unit: degree.minutes)
 */
double parse_nmea_position(nmea_position pos);

#ifdef __cplusplus
}
#endif