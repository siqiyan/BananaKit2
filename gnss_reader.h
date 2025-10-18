#ifndef __GNSS_READER_H__
#define __GNSS_READER_H__
#include <stdint.h>

#define GNSS_UART_BUF_SZ 40

typedef enum {
    M_UNKNOWN,
    M_AUTO,
    M_DIFF
} gnss_mode_t;

typedef struct {
    gnss_mode_t mode;
    float lat;
    float lon;
    float alt;
    int16_t utc_hour;
    int16_t utc_min;
    float utc_sec;
    float speed;
    int16_t year;
    int16_t month;
    int16_t day;
    uint8_t valid;
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