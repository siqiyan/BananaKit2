#ifndef __GNSS_READER_H__
#define __GNSS_READER_H__
#include <stdint.h>

#define GNSS_UART_BUF_SZ 64

typedef struct {
    uint8_t data_valid;
    int8_t debug_code0;
    int8_t debug_code1;
    int8_t debug_code2;
    int8_t debug_code3;

    int16_t latitude_degree;
    double latitude_minute;
    char latitude_hemisphere;

    int16_t longitude_degree;
    double longitude_minute;
    char longitude_hemisphere;

    int8_t utc_hour;
    int8_t utc_min;
    double utc_sec;

    int16_t year;
    int8_t month;
    int16_t day;

    char buf[GNSS_UART_BUF_SZ];
    int16_t buf_count;
} gnss_reader_t;

#define GNSS_SUCCESS        1
#define GNSS_ERR_NUL_PTR    0
#define GNSS_ERR_EMPTY      -1
#define GNSS_ERR_GPRMC      -2
#define GNSS_ERR_GPGGA      -3
#define GNSS_ERR_GPVTG      -4
#define GNSS_ERR_GPGLL      -5
#define GNSS_ERR_GPGSV      -6
#define GNSS_ERR_GPGSA      -7
#define GNSS_ERR_UNKNOWN    -10

gnss_reader_t *create_gnss_reader(void);
int destroy_gnss_reader(gnss_reader_t *gnss);
int parse_gnss_data_buf(gnss_reader_t *gnss);

#endif