#include <string.h>
#include <stdlib.h>

#include "bananakit_misc.h"
#include "gnss_reader.h"


static int parse_gprmc_string(gnss_reader_t *gnss);
static int parse_gpgga_string(gnss_reader_t *gnss);


gnss_reader_t *create_gnss_reader(void) {
    gnss_reader_t *gnss_ptr;

    gnss_ptr = (gnss_reader_t *) malloc(sizeof(gnss_reader_t));
    if(gnss_ptr == NULL) {
        return NULL;
    }

    gnss_ptr->valid = 0;
    gnss_ptr->lat = 0;
    gnss_ptr->lon = 0;
    gnss_ptr->utc_hour = 0;
    gnss_ptr->utc_min = 0;
    gnss_ptr->utc_sec = 0;
    gnss_ptr->speed = 0;
    gnss_ptr->year = 0;
    gnss_ptr->month = 0;
    gnss_ptr->day = 0;
    gnss_ptr->mode = M_UNKNOWN;
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
        return GNSS_ERR_NUL_PTR;
    }

    if(gnss->buf_count < 6) {
        return GNSS_ERR_EMPTY;
    }

    if(strncmp(gnss->buf, "$GPRMC", 6) == 0) {

        if(parse_gprmc_string(gnss) == 0) {
            return GNSS_SUCCESS;
        } else {
            return GNSS_ERR_GPRMC;
        }

    } else if(strncmp(gnss->buf, "$GPGGA", 6) == 0) {

        if(parse_gpgga_string(gnss) == 0) {
            return GNSS_SUCCESS;
        } else {
            return GNSS_ERR_GPGGA;
        }

    }
    
    // else if(strncmp(gnss->buf, "$GPVTG", 6) == 0) {

    //     // TODO
    //     return GNSS_ERR_GPVTG;

    // } else if(strncmp(gnss->buf, "$GPGLL", 6) == 0) {

    //     // TODO
    //     return GNSS_ERR_GPGLL;

    // } else if(strncmp(gnss->buf, "$GPGSV", 6) == 0) {

    //     // TODO
    //     return GNSS_ERR_GPGSV;

    // }  else if(strncmp(gnss->buf, "$GPGSA", 6) == 0) {

    //     // TODO
    //     return GNSS_ERR_GPGSA;

    // }

    return GNSS_ERR_UNKNOWN;
}

static int parse_gprmc_string(gnss_reader_t *gnss) {
    char field_buf[FIELD_BUF_SZ];
    char floatbuf[16];
    int field_sz_ret;

    // UTC time: 1
    // TODO
    // strncpy(hh, p, 2);
    // strncpy(mm, p+2, 2);
    // strncpy(ss, p+4, 5);
    // gnss->utc_hour = atoi(hh);
    // gnss->utc_min = atoi(mm);
    // gnss->utc_sec  = atof(ss);

    // Status: 2
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 2, field_buf, FIELD_BUF_SZ)) > 0) {
        gnss->valid = (field_buf[0] == 'A');
    }

    // Latitude: 3 DDMM.MMMMMMM
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 3, field_buf, FIELD_BUF_SZ)) == 12) {
        // gnss->lat = gps_atof(field_buf, field_sz_ret, 0);
    }

    // Hemisphere of latitude: 4
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 4, field_buf, FIELD_BUF_SZ)) > 0) {
        if(field_buf[0] == 'S') {
            gnss->lat *= -1;
        }
    }

    // Longitude: 5 DDDMM.MMMMMMM
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 5, field_buf, FIELD_BUF_SZ)) == 13) {
        // gnss->lon = gps_atof(field_buf, field_sz_ret, 1);
    }

    // Hemisphere of longitude: 6
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 6, field_buf, FIELD_BUF_SZ)) > 0) {
        if(field_buf[0] == 'W') {
            gnss->lon *= -1;
        }
    }

    // Speed: 7
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 7, field_buf, FIELD_BUF_SZ)) > 0) {
        // strncpy(speed, p, strchr(p, ',') - p);
        // gnss->speed = atof(speed);
    // }

    // Course over ground: 8

    // Date: 9
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 9, field_buf, FIELD_BUF_SZ)) > 0) {
        // strncpy(dd, p, 2);
        // strncpy(mo, p+2, 2);
        // strncpy(yy, p+4, 2);
        // gnss->day = atoi(dd);
        // gnss->month = atoi(mo);
        // gnss->year = 2000 + atoi(yy);
    // }

    // Magnetic variation: 10


    // Mode: 12
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 12, field_buf, FIELD_BUF_SZ)) > 0) {
        if(field_buf[0] == 'A') {
            gnss->mode = M_AUTO;
        } else if(field_buf[0] == 'D') {
            gnss->mode = M_DIFF;
        } else {
            gnss->mode = M_UNKNOWN;
        }
    }

    // Checksum: 13
    return 0;
}

static int parse_gpgga_string(gnss_reader_t *gnss) {
    // const char *p = gnss->buf;
    // char hh[3], mm[3], ss[6];
    // char lat[8];
    // char lon[8];
    // char degree[8], minute[8];

    char field_buf[FIELD_BUF_SZ];
    char floatbuf[16];
    int field_sz_ret;

    // UTC time: 1
    // if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 1, field_buf, FIELD_BUF_SZ)) > 0) {
    // }

    // Latitude: 2
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 2, field_buf, FIELD_BUF_SZ)) == 12) {
        // gnss->lat = gps_atof(field_buf, field_sz_ret, 0);
    }

    // Hemisphere of latitude: 3
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 3, field_buf, FIELD_BUF_SZ)) > 0) {
        if(field_buf[0] == 'S') {
            gnss->lat *= -1;
        }
    }

    // Longitude: 4 DDDMM.MMMMMMM
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 4, field_buf, FIELD_BUF_SZ)) == 13) {
        // gnss->lon = gps_atof(field_buf, field_sz_ret, 0);
    }

    // Hemisphere of longitude: 5
    if((field_sz_ret = field_extract(gnss->buf, gnss->buf_count, ',', 5, field_buf, FIELD_BUF_SZ)) > 0) {
        if(field_buf[0] == 'W') {
            gnss->lon *= -1;
        }
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