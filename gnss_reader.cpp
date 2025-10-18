#include <string.h>
#include <stdlib.h>

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

    } else if(strncmp(gnss->buf, "$GPVTG", 6) == 0) {

        // TODO
        return GNSS_ERR_GPVTG;

    } else if(strncmp(gnss->buf, "$GPGLL", 6) == 0) {

        // TODO
        return GNSS_ERR_GPGLL;

    } else if(strncmp(gnss->buf, "$GPGSV", 6) == 0) {

        // TODO
        return GNSS_ERR_GPGSV;

    } else if(strncmp(gnss->buf, "$GPGGA", 6) == 0) {

        if(parse_gpgga_string(gnss) == 0) {
            return GNSS_SUCCESS;
        } else {
            return GNSS_ERR_GPRMC;
        }

    } else if(strncmp(gnss->buf, "$GPGSA", 6) == 0) {

        // TODO
        return GNSS_ERR_GPGSA;

    }

    return GNSS_ERR_UNKNOWN;
}

static int parse_gprmc_string(gnss_reader_t *gnss) {
    const char *p = gnss->buf;
    char hh[3], mm[3], ss[6];
    char lat[8];
    char lon[8];
    // char speed[32];
    char dd[3], mo[3], yy[3];
    char degree[8], minute[8];
    for(int i = 1; i <= 13; i++) {
        p = strchr(p, ',');
        if(p == NULL) {
            return -1;
        }
        p++; // point to the start of new colume
        switch(i) {
            case 0:
                // Sentence ID
                break;

            case 1:
                // UTC time
                strncpy(hh, p, 2);
                strncpy(mm, p+2, 2);
                strncpy(ss, p+4, 5);
                gnss->utc_hour = atoi(hh);
                gnss->utc_min = atoi(mm);
                gnss->utc_sec  = atof(ss);
                break;

            case 2:
                // Status
                gnss->valid = (*p == 'A');
                break;

            case 3:
                // Latitude p: DDMM.MMMMMMM,
                strncpy(lat, p, strchr(p, ',') - p);
                strncpy(degree, lat, 2);
                strcpy(minute, lat+2);
                gnss->lat = atof(degree) + atof(minute) / 60.0;
                break;

            case 4:
                // Hemisphere of latitude
                if(*p == 'S') {
                    gnss->lat *= -1;
                }
                break;

            case 5:
                // Longitude p: DDDMM.MMMMMMM,
                strncpy(lon, p, strchr(p, ',') - p);
                strncpy(degree, lon, 3);
                strcpy(minute, lon+3);
                gnss->lon = atof(degree) + atof(minute) / 60.0;
                break;

            case 6:
                // Hemisphere of longitude
                if(*p == 'W') {
                    gnss->lon *= -1;
                }
                break;

            case 7:
                // Speed
                // strncpy(speed, p, strchr(p, ',') - p);
                // gnss->speed = atof(speed);
                break;

            case 8:
                // Course over ground
                break;

            case 9:
                // Date
                strncpy(dd, p, 2);
                strncpy(mo, p+2, 2);
                strncpy(yy, p+4, 2);
                gnss->day = atoi(dd);
                gnss->month = atoi(mo);
                gnss->year = 2000 + atoi(yy);
                break;

            case 10:
                // Magnetic variation
                break;

            case 11:
                // ???
                break;

            case 12:
                // Mode
                if(*p == 'A') {
                    gnss->mode = M_AUTO;
                } else if(*p == 'D') {
                    gnss->mode = M_DIFF;
                } else {
                    gnss->mode = M_UNKNOWN;
                }
                break;

            case 13:
                // Checksum
                break;

            default:
                // fprintf(
                //     stderr,
                //     "Unknown colume number detected i=%d\n",
                //     i
                // );
                return -1;
        }
    }
    return 0;
}

static int parse_gpgga_string(gnss_reader_t *gnss) {
    const char *p = gnss->buf;
    char hh[3], mm[3], ss[6];
    char lat[8];
    char lon[8];
    char degree[8], minute[8];
    for(int i = 1; i <= 13; i++) {
        p = strchr(p, ',');
        if(p == NULL) {
            return -1;
        }
        p++; // point to the start of new colume
        switch(i) {
            case 0:
                // Sentence ID
                break;

            case 1:
                // UTC time
                strncpy(hh, p, 2);
                strncpy(mm, p+2, 2);
                strncpy(ss, p+4, 5);
                gnss->utc_hour = atoi(hh);
                gnss->utc_min = atoi(mm);
                gnss->utc_sec  = atof(ss);
                break;

            case 2:
                // Latitude p: DDMM.MMMMMMM,
                strncpy(lat, p, strchr(p, ',') - p);
                strncpy(degree, lat, 2);
                strcpy(minute, lat+2);
                gnss->lat = atof(degree) + atof(minute) / 60.0;
                break;

            case 3:
                // Hemisphere of latitude
                if(*p == 'S') {
                    gnss->lat *= -1;
                }
                break;

            case 4:
                // Longitude p: DDDMM.MMMMMMM,
                strncpy(lon, p, strchr(p, ',') - p);
                strncpy(degree, lon, 3);
                strcpy(minute, lon+3);
                gnss->lon = atof(degree) + atof(minute) / 60.0;
                break;

            case 5:
                // Hemisphere of longitude
                if(*p == 'W') {
                    gnss->lon *= -1;
                }
                break;
            
            case 6:
                // Position fix indicator
                break;

            case 7:
                // Sat used:
                break;

            case 8:
                // hdop
                break;

            case 9:
                // msl altitude
                break;

            case 10:
                // units
                break;

            case 11:
                // geoid seperation
                break;

            case 12:
                // unit
                break;

            case 13:
                // age of diff corr
                break;

            case 14:
                // diff ref station id
                break;

            case 15:
                // checksum
                break;

            default:
                return -1;
        }
    }
    return 0;
}