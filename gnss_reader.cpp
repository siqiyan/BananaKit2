#include <string.h>
#include <stdlib.h>

#include "bananakit_misc.h"
#include "gnss_reader.h"

#define FIELD_BUF_SZ 16
#define DIGIT_BUF_SZ 16

static int parse_gprmc_string(gnss_reader_t *gnss);
static int parse_gpgga_string(gnss_reader_t *gnss);
static int convert_verify_latlon(
    const char *field,
    int n,
    int ndd,
    int16_t *degree,
    double *minute
);
static int convert_verify_utc_time(
    const char *field,
    int n,
    int8_t *hour,
    int8_t *minute,
    double *second
);
static int convert_verify_date(
    const char *field,
    int n,
    int16_t *year,
    int8_t *month,
    int16_t *day
);

gnss_reader_t *create_gnss_reader(void) {
    gnss_reader_t *gnss_ptr;

    gnss_ptr = (gnss_reader_t *) malloc(sizeof(gnss_reader_t));
    if(gnss_ptr == NULL) {
        return NULL;
    }

    gnss_ptr->data_valid = 0;
    gnss_ptr->latitude_degree = 0;
    gnss_ptr->latitude_minute = 0.0;
    gnss_ptr->latitude_hemisphere = '\0';
    gnss_ptr->longitude_degree = 0;
    gnss_ptr->longitude_minute = 0.0;
    gnss_ptr->longitude_hemisphere = '\0';
    gnss_ptr->utc_hour = 0;
    gnss_ptr->utc_min = 0;
    gnss_ptr->utc_sec = 0.0;
    gnss_ptr->year = 0;
    gnss_ptr->month = 0;
    gnss_ptr->day = 0;
    gnss_ptr->buf[0] = '\0';
    gnss_ptr->buf_count = 0;

    return gnss_ptr;
}

int destroy_gnss_reader(gnss_reader_t *gnss) {
    if(gnss != NULL) {
        free(gnss);
        return GNSS_SUCCESS;
    } else {
        return GNSS_ERR_NUL_PTR;
    }
}

int parse_gnss_data_buf(gnss_reader_t *gnss) {
    if(gnss == NULL) {
        return 0;
    }

    if(gnss->buf_count < 6) {
        gnss->debug_code0 = -1;
        return -1;
    }

    if(strncmp(gnss->buf, "$GPRMC", 6) == 0) {
        if(parse_gprmc_string(gnss) == 1) {
            gnss->debug_code0 = 2;
            return 2;
        } else {
            gnss->debug_code0 = -2;
            return -2;
        }
    } else if(strncmp(gnss->buf, "$GPGGA", 6) == 0) {
        if(parse_gpgga_string(gnss) == 1) {
            gnss->debug_code0 = 3;
            return 3;
        } else {
            gnss->debug_code0 = -3;
            return -3;
        }
    } else if(strncmp(gnss->buf, "$GPVTG", 6) == 0) {

        // TODO
        gnss->debug_code0 = -4;
        return -4;

    } else if(strncmp(gnss->buf, "$GPGLL", 6) == 0) {

        // TODO
        gnss->debug_code0 = -5;
        return -5;

    } else if(strncmp(gnss->buf, "$GPGSV", 6) == 0) {

        // TODO
        gnss->debug_code0 = -6;
        return -6;

    }  else if(strncmp(gnss->buf, "$GPGSA", 6) == 0) {

        // TODO
        gnss->debug_code0 = -7;
        return -7;

    }

    gnss->debug_code0 = -8;
    return -8;
}

static int parse_gprmc_string(gnss_reader_t *gnss) {
    char field_buf[FIELD_BUF_SZ];
    int field_sz;
    int ret_code;

    gnss->data_valid = 1;

    // Hemisphere of latitude
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 4, field_buf, FIELD_BUF_SZ);
    if((field_sz) > 0) {
        gnss->latitude_hemisphere = field_buf[0];
    } else {
        gnss->latitude_hemisphere = '\0';
        gnss->data_valid = 0;
        gnss->debug_code1 = -1;
        return -1;
    }

    // Hemisphere of longitude
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 6, field_buf, FIELD_BUF_SZ);
    if(field_sz > 0) {
        gnss->longitude_hemisphere = field_buf[0];
    } else {
        gnss->longitude_hemisphere = '\0';
        gnss->data_valid = 0;
        gnss->debug_code1 = -2;
        return -2;
    }

    // Latitude: 3 DDMM.MMMMMMM
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 3, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 7) {
        ret_code = convert_verify_latlon(
            field_buf,
            field_sz,
            2,
            &gnss->latitude_degree,
            &gnss->latitude_minute
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -3;
            gnss->debug_code2 = ret_code;
            return -3;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -4;
        return -4;
    }

    // Longitude: 5 DDDMM.MMMMMMM
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 5, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 8) {
        ret_code = convert_verify_latlon(
            field_buf,
            field_sz,
            3,
            &gnss->longitude_degree,
            &gnss->longitude_minute
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -5;
            gnss->debug_code2 = ret_code;
            return -5;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -6;
        return -6;
    }

    // UTC time: 1
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 1, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 6) {
        ret_code = convert_verify_utc_time(
            field_buf,
            field_sz,
            &gnss->utc_hour,
            &gnss->utc_min,
            &gnss->utc_sec
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -7;
            gnss->debug_code2 = ret_code;
            return -7;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -8;
        return -8;
    }

    // // Date: 9
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 9, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 6) {
        ret_code = convert_verify_date(
            field_buf,
            field_sz,
            &gnss->year,
            &gnss->month,
            &gnss->day
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -9;
            gnss->debug_code2 = ret_code;
            return -9;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -10;
        return -10;
    }

    return 1;
}

static int parse_gpgga_string(gnss_reader_t *gnss) {
    char field_buf[FIELD_BUF_SZ];
    int field_sz;
    int ret_code;

    gnss->data_valid = 1;

    // UTC time: 1
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 1, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 6) {
        ret_code = convert_verify_utc_time(
            field_buf,
            field_sz,
            &gnss->utc_hour,
            &gnss->utc_min,
            &gnss->utc_sec
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -7;
            gnss->debug_code2 = ret_code;
            return -7;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -8;
        return -8;
    }

    // Latitude: 2
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 2, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 7) {
        ret_code = convert_verify_latlon(
            field_buf,
            field_sz,
            2,
            &gnss->latitude_degree,
            &gnss->latitude_minute
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -3;
            gnss->debug_code2 = ret_code;
            return -3;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -4;
        return -4;
    }

    // Hemisphere of latitude: 3
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 3, field_buf, FIELD_BUF_SZ);
    if((field_sz) > 0) {
        gnss->latitude_hemisphere = field_buf[0];
    } else {
        gnss->latitude_hemisphere = '\0';
        gnss->data_valid = 0;
        gnss->debug_code1 = -1;
        return -1;
    }

    // Longitude: 4 DDDMM.MMMMMMM
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 4, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 8) {
        ret_code = convert_verify_latlon(
            field_buf,
            field_sz,
            3,
            &gnss->longitude_degree,
            &gnss->longitude_minute
        );
        if(ret_code != 1) {
            gnss->data_valid = 0;
            gnss->debug_code1 = -5;
            gnss->debug_code2 = ret_code;
            return -5;
        }
    } else {
        gnss->data_valid = 0;
        gnss->debug_code1 = -6;
        return -6;
    }

    // Hemisphere of longitude: 5
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 5, field_buf, FIELD_BUF_SZ);
    if(field_sz > 0) {
        gnss->longitude_hemisphere = field_buf[0];
    } else {
        gnss->longitude_hemisphere = '\0';
        gnss->data_valid = 0;
        gnss->debug_code1 = -2;
        return -2;
    }

    // Position fix indicator: 6
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 6, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // Sat used: 7
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 7, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // hdop: 8
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 8, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // msl altitude: 9
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 9, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // units: 10
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 10, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // geoid seperation: 11
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 11, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // unit: 12
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 12, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // age of diff corr: 13
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 13, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // diff ref station id: 14
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 14, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // checksum: 15
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 15, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    return 0;
}

static int convert_verify_latlon(
    const char *field,
    int n,
    int ndd,
    int16_t *degree,
    double *minute
) {
    // Convert and verify string to latitude(in degree and minute)
    // or longitude (also in degree and minute) depend on format:
    // The format is determined by ndd:
    // ndd == 2: "DDMM.MMMMMMM"
    // ndd == 3: "DDDMM.MMMMMMM"

    char digitbuf[DIGIT_BUF_SZ];
    double fvalue;
    int i, j;    

    // Pre-checking:
    if(field == NULL || degree == NULL || minute == NULL) {
        // Error: invalid pointer
        return 0;
    }
    if(n <= 0) {
        // Error: string size invalid
        return -1;
    }
    if(ndd >= n) {
        // Error: ndd invalid 
        return -2;
    }
    if(n > DIGIT_BUF_SZ - 1) {
        // Error: digit buffer size too small
        return -3;
    }

    // Process the DD/DDD part:
    for(i = 0; i < ndd; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[i] = field[i];
        } else {
            // Error: invalid character found
            return -4;
        }
    }
    digitbuf[i] = '\0';
    *degree = atoi(digitbuf);

    // Process the MM.MMM... part:
    for(i = ndd, j = 0; i < n; i++) {
        if(IS_DIGIT(field[i]) || field[i] == '.') {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return -5;
        }
    }
    digitbuf[j] = '\0';
    *minute = atof(digitbuf);

    return 1; // Success
}

#define IDX_MINUTE 2
#define IDX_SECOND 4
static int convert_verify_utc_time(
    const char *field,
    int n,
    int8_t *hour,
    int8_t *minute,
    double *second
) {
    char digitbuf[DIGIT_BUF_SZ];
    int i, j;

    // Pre-checking:
    if(field == NULL || hour == NULL || minute == NULL || second == NULL) {
        // Error: invalid pointer
        return 0;
    }
    if(n <= 0) {
        // Error: string size invalid
        return -1;
    }
    if(n > DIGIT_BUF_SZ - 1) {
        // Error: digit buffer size too small
        return -2;
    }

    for(i = 0; i < IDX_MINUTE; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[i] = field[i];
        } else {
            // Error: invalid character found
            return -3;
        }
    }
    digitbuf[i] = '\0';
    *hour = atoi(digitbuf);

    for(i = IDX_MINUTE, j = 0; i < IDX_SECOND; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return -4;
        }
    }
    digitbuf[j] = '\0';
    *minute = atoi(digitbuf);

    for(i = IDX_SECOND, j = 0; i < n; i++) {
        if(IS_DIGIT(field[i] || field[i] == '.')) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return -5;
        }
    }
    digitbuf[j] = '\0';
    *second = atof(digitbuf);

    return 1; // Success

    // strncpy(hh, p, 2);
    // strncpy(mm, p+2, 2);
    // strncpy(ss, p+4, 5);
    // gnss->utc_hour = atoi(hh);
    // gnss->utc_min = atoi(mm);
    // gnss->utc_sec  = atof(ss);
}

#define IDX_MONTH 2
#define IDX_YEAR 4
static int convert_verify_date(
    const char *field,
    int n,
    int16_t *year,
    int8_t *month,
    int16_t *day
) {
    char digitbuf[DIGIT_BUF_SZ];
    int i, j;

    // Pre-checking:
    if(field == NULL || year == NULL || month == NULL || day == NULL) {
        // Error: invalid pointer
        return 0;
    }
    if(n <= 0) {
        // Error: string size invalid
        return -1;
    }
    if(n > DIGIT_BUF_SZ - 1) {
        // Error: digit buffer size too small
        return -2;
    }

    for(i = 0; i < IDX_MONTH; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[i] = field[i];
        } else {
            // Error: invalid character found
            return -3;
        }
    }
    digitbuf[i] = '\0';
    *day = atoi(digitbuf);

    for(i = IDX_MONTH, j = 0; i < IDX_YEAR; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return -4;
        }
    }
    digitbuf[j] = '\0';
    *month = atoi(digitbuf);

    for(i = IDX_YEAR, j = 0; i < n; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return -5;
        }
    }
    digitbuf[j] = '\0';
    *year = atoi(digitbuf) + 2000;

    return 1; // Success

    // strncpy(dd, p, 2);
    // strncpy(mo, p+2, 2);
    // strncpy(yy, p+4, 2);
    // gnss->day = atoi(dd);
    // gnss->month = atoi(mo);
    // gnss->year = 2000 + atoi(yy);
}