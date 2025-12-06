#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "banana_string.h"
#include "gnss_reader.h"

#define FIELD_BUF_SZ 16
#define DIGIT_BUF_SZ 16

static int parse_gprmc_string(gnss_reader_t *gnss);
static int parse_gpgga_string(gnss_reader_t *gnss);
static int convert_verify_latlon(const char *field, int n, int ndd, int16_t *degree, double *minute);
static int convert_verify_utc_time(const char *field, int n, int8_t *hour, int8_t *minute, double *second);
static int convert_verify_date(const char *field, int n, int16_t *year, int8_t *month, int16_t *day);

gnss_reader_t *create_gnss_reader(void) {
    gnss_reader_t *gnss_ptr;
    gnss_ptr = (gnss_reader_t *) malloc(sizeof(gnss_reader_t));
    if(gnss_ptr != NULL) {
        gnss_ptr->status.data_valid         = 0;
        gnss_ptr->status.data_initialized   = 0;
        gnss_ptr->status.nmea_started       = 0;
        gnss_ptr->status.origin_initialized = 0;
        gnss_ptr->buf[0] = '\0';
        gnss_ptr->buf_count = 0;
        gnss_ptr->debug_code = 0;
    }
    return gnss_ptr;
}

int destroy_gnss_reader(gnss_reader_t *gnss) {
    if(gnss != NULL) {
        free(gnss);
        return 1;
    } else {
        return 0;
    }
}

int gnss_update(gnss_reader_t *gnss, char c) {
    // Append incoming data into buffer and process if '$' is detected or
    // if the buffer is full.

    if(gnss == NULL) {
        return 0;
    }

    if(c == '$') {

        if(gnss->status.nmea_started) {
            gnss->buf[gnss->buf_count] = '\0';
            parse_gnss_data_buf(gnss);
        }

        gnss->buf_count = 0;
        gnss->buf[gnss->buf_count++] = '$';
        gnss->status.nmea_started = 1;

    } else {

        if(gnss->buf_count >= GNSS_UART_BUF_SZ - 1) {

            if(gnss->status.nmea_started) {
                gnss->buf[gnss->buf_count] = '\0';
                parse_gnss_data_buf(gnss);
            }

            gnss->buf_count = 0;
            gnss->buf[0] = '\0';
            gnss->status.nmea_started = 0;
            
        }

        gnss->buf[gnss->buf_count++] = c;

    }

    return 1;
}

int parse_gnss_data_buf(gnss_reader_t *gnss) {
    // Return: bool

    if(gnss == NULL) {
        return 0;
    }

    gnss->debug_code = 0;

    if(gnss->buf_count < 6) {
        // Error: data in buffer too short
        gnss->debug_code = 1;
        return 0;
    }

    if(strncmp(gnss->buf, "$GPRMC", 6) == 0) {
        return parse_gprmc_string(gnss);
    } else if(strncmp(gnss->buf, "$GPGGA", 6) == 0) {
        return parse_gpgga_string(gnss);
    }
    
    // Reserved for parsing other NMEA setences:
    // else if(strncmp(gnss->buf, "$GPVTG", 6) == 0) {
    // } else if(strncmp(gnss->buf, "$GPGLL", 6) == 0) {
    // } else if(strncmp(gnss->buf, "$GPGSV", 6) == 0) {
    // }  else if(strncmp(gnss->buf, "$GPGSA", 6) == 0) {
    // }

    return 1;
}

static int parse_gprmc_string(gnss_reader_t *gnss) {
    // Parse Recommended Minimum Specific (RMC) GNSS data
    // Return: bool

    char field_buf[FIELD_BUF_SZ];
    int field_sz;
    int ret_code;

    gnss->status.data_valid = 1;

    // UTC time
#ifdef ENABLE_TIME_PARSE
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 1, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 6) {
        ret_code = convert_verify_utc_time(
            field_buf,
            field_sz,
            &gnss->utc_hour,
            &gnss->utc_min,
            &gnss->utc_sec
        );
        if(ret_code != 0) {
            // Error: fail to parse utc time
            gnss->status.data_valid = 0;
            gnss->debug_code = 2;
            return 0;
        }
    } else {
        // Error: utc time field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 3;
        return 0;
    }
#endif

    // Latitude: DDMM.MMMMMMM
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 3, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 7) {
        ret_code = convert_verify_latlon(field_buf, field_sz, 2, &gnss->lat.degree, &gnss->lat.minute);
        if(ret_code != 0) {
            // Error: fail to parse latitude
            gnss->status.data_valid = 0;
            gnss->debug_code = 4;
            return 0;
        }
    } else {
        // Error: latitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 5;
        return 0;
    }

    // Hemisphere of latitude
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 4, field_buf, FIELD_BUF_SZ);
    if(field_sz > 0) {
        if(!set_hemisphere(&gnss->lat, field_buf[0], 1)) {
            // Error: fail to parse hemisphere of latitude
            gnss->status.data_valid = 0;
            gnss->debug_code = 6;
            return 0;
        }
    } else {
        // Error: hemisphere of latitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 7;
        return 0;
    }

    // Longitude: DDDMM.MMMMMMM
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 5, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 8) {
        ret_code = convert_verify_latlon(field_buf, field_sz, 3, &gnss->lon.degree, &gnss->lon.minute);
        if(ret_code != 0) {
            // Error: fail to parse longitude field
            gnss->status.data_valid = 0;
            gnss->debug_code = 8;
            return 0;
        }
    } else {
        // Error: longitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 9;
        return 0;
    }

    // Hemisphere of longitude
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 6, field_buf, FIELD_BUF_SZ);
    if(field_sz > 0) {
        if(!set_hemisphere(&gnss->lon, field_buf[0], 0)) {
            // Error: failed to parse hemisphere of longitude
            gnss->status.data_valid = 0;
            gnss->debug_code = 10;
            return 0;
        }
    } else {
        // Error: hemisphere of longitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 11;
        return 0;
    }

    // Date
#ifdef ENABLE_TIME_PARSE
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 9, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 6) {
        ret_code = convert_verify_date(
            field_buf,
            field_sz,
            &gnss->year,
            &gnss->month,
            &gnss->day
        );
        if(ret_code != 0) {
            // Error: failed to parse date field
            gnss->status.data_valid = 0;
            gnss->debug_code = 12;
            return 0;
        }
    } else {
        // Error: date field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 13;
        return 0;
    }
#endif

    if(gnss->status.data_valid) {
        gnss->status.data_initialized = 1;
    }

    return 1;
}

static int parse_gpgga_string(gnss_reader_t *gnss) {
    // Parse Global Positioning System Fixed Data:
    // (Not sure why it called GPGGA)
    // Return: bool

    char field_buf[FIELD_BUF_SZ];
    int field_sz;
    int ret_code;

    gnss->status.data_valid = 1;

    // UTC time: 1
#ifdef ENABLE_TIME_PARSE
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 1, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 6) {
        ret_code = convert_verify_utc_time(
            field_buf,
            field_sz,
            &gnss->utc_hour,
            &gnss->utc_min,
            &gnss->utc_sec
        );
        if(ret_code != 0) {
            // Error: failed to parse utc time
            gnss->status.data_valid = 0;
            gnss->debug_code = 14;
            return 0;
        }
    } else {
        // Error: utc time field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 15;
        return 0;
    }
#endif

    // Latitude: 2
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 2, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 7) {
        ret_code = convert_verify_latlon(field_buf, field_sz, 2, &gnss->lat.degree, &gnss->lat.minute);
        if(ret_code != 0) {
            // Error: failed to parse latitude field
            gnss->status.data_valid = 0;
            gnss->debug_code = 16;
            return 0;
        }
    } else {
        // Error: latitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 17;
        return 0;
    }

    // Hemisphere of latitude: 3
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 3, field_buf, FIELD_BUF_SZ);
    if(field_sz > 0) {
        if(!set_hemisphere(&gnss->lat, field_buf[0], 1)) {
            // Error: failed to parse hemisphere of latitude field
            gnss->status.data_valid = 0;
            gnss->debug_code = 18;
            return 0;
        }
    } else {
        // Error: hemisphere of latitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 19;
        return 0;
    }

    // Longitude: 4 DDDMM.MMMMMMM
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 4, field_buf, FIELD_BUF_SZ);
    if(field_sz >= 8) {
        ret_code = convert_verify_latlon(field_buf, field_sz, 3, &gnss->lon.degree, &gnss->lon.minute);
        if(ret_code != 0) {
            // Error: failed to parse longitude field
            gnss->status.data_valid = 0;
            gnss->debug_code = 20;
            return 0;
        }
    } else {
        // Error: longitude field too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 21;
        return 0;
    }

    // Hemisphere of longitude: 5
    field_sz = field_extract(gnss->buf, gnss->buf_count, ',', 5, field_buf, FIELD_BUF_SZ);
    if(field_sz > 0) {
        if(!set_hemisphere(&gnss->lon, field_buf[0], 0)) {
            // Error: failed to parse hemisphere of longitude field
            gnss->status.data_valid = 0;
            gnss->debug_code = 22;
            return 0;
        }
    } else {
        // Error: hemisphere of longitude too short
        gnss->status.data_valid = 0;
        gnss->debug_code = 23;
        return 0;
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

    if(gnss->status.data_valid) {
        gnss->status.data_initialized = 1;
    }

    return 1;
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
    // Return:
    //  success - 0
    //  other   - error code > 0

    char digitbuf[DIGIT_BUF_SZ];
    double fvalue;
    int i, j;    

    // Pre-checking:
    if(field == NULL || degree == NULL || minute == NULL) {
        // Error: invalid pointer
        return 1;
    }
    if(n <= 0) {
        // Error: string size invalid
        return 2;
    }
    if(ndd >= n) {
        // Error: ndd invalid 
        return 3;
    }
    if(n > DIGIT_BUF_SZ - 1) {
        // Error: digit buffer size too small
        return 4;
    }

    // Process the DD/DDD part:
    for(i = 0; i < ndd; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[i] = field[i];
        } else {
            // Error: invalid character found
            return 5;
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
            return 6;
        }
    }
    digitbuf[j] = '\0';
    *minute = atof(digitbuf);

    return 0; // Success
}

#ifdef ENABLE_TIME_PARSE
#define IDX_MINUTE 2
#define IDX_SECOND 4
static int convert_verify_utc_time(const char *field, int n, int8_t *hour, int8_t *minute, double *second) {
    // Return:
    //  success - 0
    //  other   - error code > 0
    char digitbuf[DIGIT_BUF_SZ];
    int i, j;

    // Pre-checking:
    if(field == NULL || hour == NULL || minute == NULL || second == NULL) {
        // Error: invalid pointer
        return 1;
    }
    if(n <= 0) {
        // Error: string size invalid
        return 2;
    }
    if(n > DIGIT_BUF_SZ - 1) {
        // Error: digit buffer size too small
        return 3;
    }

    for(i = 0; i < IDX_MINUTE; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[i] = field[i];
        } else {
            // Error: invalid character found
            return 4;
        }
    }
    digitbuf[i] = '\0';
    *hour = atoi(digitbuf);

    for(i = IDX_MINUTE, j = 0; i < IDX_SECOND; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return 5;
        }
    }
    digitbuf[j] = '\0';
    *minute = atoi(digitbuf);

    for(i = IDX_SECOND, j = 0; i < n; i++) {
        if(IS_DIGIT(field[i] || field[i] == '.')) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return 6;
        }
    }
    digitbuf[j] = '\0';
    *second = atof(digitbuf);

    return 0; // Success
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
    // Return:
    //  success - 0
    //  other   - error code > 0
    char digitbuf[DIGIT_BUF_SZ];
    int i, j;

    // Pre-checking:
    if(field == NULL || year == NULL || month == NULL || day == NULL) {
        // Error: invalid pointer
        return 1;
    }
    if(n <= 0) {
        // Error: string size invalid
        return 2;
    }
    if(n > DIGIT_BUF_SZ - 1) {
        // Error: digit buffer size too small
        return 3;
    }

    for(i = 0; i < IDX_MONTH; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[i] = field[i];
        } else {
            // Error: invalid character found
            return 4;
        }
    }
    digitbuf[i] = '\0';
    *day = atoi(digitbuf);

    for(i = IDX_MONTH, j = 0; i < IDX_YEAR; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return 5;
        }
    }
    digitbuf[j] = '\0';
    *month = atoi(digitbuf);

    for(i = IDX_YEAR, j = 0; i < n; i++) {
        if(IS_DIGIT(field[i])) {
            digitbuf[j++] = field[i];
        } else {
            // Error: invalid character found
            return 6;
        }
    }
    digitbuf[j] = '\0';
    *year = atoi(digitbuf) + 2000;

    return 0; // Success
}
#endif

int set_origin(gnss_reader_t *gnss) {
    // Modified from Gemini generated code
    // Return: bool

    if(gnss->status.data_initialized) {
        gnss->origin_lat = gnss->lat;
        gnss->origin_lon = gnss->lon;

        // Calculate total decimal latitude for the cosine calculation only
        // We only do this ONCE to save processing power
        gnss->ref_lat_dec = gnss->origin_lat.degree + (gnss->origin_lat.minute / 60.0);

        // Pre-calculate the longitude scaling factor based on current latitude
        // conversion: degrees to radians for cos function
        gnss->meters_per_deg_lon = METERS_PER_DEGREE_LAT * cos(gnss->ref_lat_dec * M_PI / 180.0);

        gnss->status.origin_initialized = 1;
        return 1; // success
    } else {
        return 0; // failed
    }
}

int gnss_update_local_xy(gnss_reader_t *gnss) {
    // Return: bool
    if((!gnss->status.origin_initialized) || (!gnss->status.data_initialized) || (!gnss->status.data_valid)) {
        return 0; // failed
    }
    return gps2localxy(gnss, &gnss->lat, &gnss->lon, &gnss->local_x, &gnss->local_y);
}

int gps2localxy(const gnss_reader_t *gnss, const geo_coordinate *lat, geo_coordinate *lon, double *x, double *y) {
    // Modified from Gemini generated code

    if((!gnss->status.origin_initialized) || (!gnss->status.data_initialized)) {
        return 0; // failed
    }

    // 1. Calculate the difference in DEGREES
    int16_t deg_diff_lat = lat->degree - gnss->origin_lat.degree;
    int16_t deg_diff_lon = lon->degree - gnss->origin_lon.degree;

    // 2. Calculate the difference in MINUTES
    double min_diff_lat = lat->minute - gnss->origin_lat.minute;
    double min_diff_lon = lon->minute - gnss->origin_lon.minute;

    // 3. Combine them into a single "Delta Degree" float
    // This number is SMALL, so we don't lose precision
    double total_delta_lat = deg_diff_lat + (min_diff_lat / 60.0);
    double total_delta_lon = deg_diff_lon + (min_diff_lon / 60.0);

    // 4. Convert to Meters (Local X, Y)
    // Y is North-South, X is East-West
    *y = total_delta_lat * METERS_PER_DEGREE_LAT;
    *x = total_delta_lon * gnss->meters_per_deg_lon;

    return 1;
}

int set_hemisphere(geo_coordinate *coord, char letter, int is_lat) {
    // To help clarify the hemisphere letters and sign of geo coordinate.
    // According to Gemini:
    // The standard convention used in GPS, WGS84, and almost all digital navigation systems is:
    // The Standard Sign Convention
    // Latitude (North/South):
    // North (+): Positive (e.g., New York is +40)
    // South (−): Negative (e.g., Sydney is -33)
    // Longitude (East/West):
    // East (+): Positive (e.g., Tokyo is +139)
    // West (−): Negative (e.g., Los Angeles is -118)
    // Return: bool
    //  success - 1
    //  fail    - 0
    if(is_lat) {
        // Latitude
        if(letter == 'N' || letter == 'n') {
            coord->sign = 1;
        } else if(letter == 'S' || letter == 's') {
            coord->sign = -1;
        } else {
            coord->sign = 0; // error
            return 0;
        }
    } else {
        // Longitude
        if(letter == 'E' || letter == 'e') {
            coord->sign = 1;
        } else if(letter == 'W' || letter == 'w') {
            coord->sign = -1;
        } else {
            coord->sign = 0; // error
            return 0;
        }
    }
    return 1;
}