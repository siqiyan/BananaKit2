#ifndef __GNSS_READER_H__
#define __GNSS_READER_H__
#include <stdint.h>

#define ENABLE_TIME_PARSE // toggle time parsing on/off
#define GNSS_UART_BUF_SZ 64
#define METERS_PER_DEGREE_LAT 111132.92

typedef struct {
    int8_t sign;
    int16_t degree;
    double minute;
} geo_coordinate;

typedef struct {
    uint8_t data_valid          : 1;
    uint8_t data_initialized    : 1;
    uint8_t nmea_started        : 1;
    uint8_t origin_initialized  : 1;
} gnss_status_code_t;

typedef struct {
    gnss_status_code_t  status;
    uint8_t debug_code;

    geo_coordinate lat;
    geo_coordinate lon;
    geo_coordinate origin_lat;
    geo_coordinate origin_lon;
    double ref_lat_dec;         // used to compute local coordinates
    double meters_per_deg_lon;  // used to compute local coordinates

    double local_x;
    double local_y;

#ifdef ENABLE_TIME_PARSE
    int8_t utc_hour;
    int8_t utc_min;
    double utc_sec;

    int16_t year;
    int8_t month;
    int16_t day;
#endif

    char buf[GNSS_UART_BUF_SZ];
    int16_t buf_count;
} gnss_reader_t;

gnss_reader_t *create_gnss_reader(void);
int destroy_gnss_reader(gnss_reader_t *gnss);
int parse_gnss_data_buf(gnss_reader_t *gnss);
int gnss_update(gnss_reader_t *gnss, char c);
int set_hemisphere(geo_coordinate *coord, char letter, int is_lat);
int set_origin(gnss_reader_t *gnss);
int gnss_update_local_xy(gnss_reader_t *gnss);

#endif