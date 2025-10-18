#include <string.h>
#include <stdlib.h>
#include <SoftwareSerial.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"
#include "gps_imu_module.h"
#include "gnss_reader.h"

extern callstack_t Callstack;
extern bananakit_io_t IO;

SoftwareSerial GPS_Serial(GT_U7_TX, GT_U7_RX);
gnss_reader_t *GNSS;


static void refresh_display(void);


void gps_imu_module_init(void) {
    pinMode(GT_U7_TX, INPUT);
    pinMode(GT_U7_RX, OUTPUT);

    GNSS = create_gnss_reader();
    GPS_Serial.begin(9600);
    GPS_Serial.listen();

    // Serial.begin(9600);
}

node_status_t gps_imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    int gnss_ret;
    char c;
    uint8_t nmea_start;

    if(GNSS == NULL) {
        snprintf(
            IO.lcd_buf0,
            LCD_BUF_SIZE,
            "GNSS init err"
        );
        return NODE_EXITING;
    }

    // if(GPS_Serial.available()) {
    //     char c = GPS_Serial.read();
    //     Serial.write(c);
    // }

    GNSS->buf_count = 0;
    nmea_start = 0;
    while(GPS_Serial.available() && GNSS->buf_count < GNSS_UART_BUF_SZ - 1) {
        c = GPS_Serial.read();
        if(c == '$') {
            nmea_start = 1;
        }
        if(!nmea_start) {
            continue;
        }
        GNSS->buf[GNSS->buf_count++] = c;
    }
    GNSS->buf[GNSS->buf_count++] = '\0'; // set nul character
    gnss_ret = parse_gnss_data_buf(GNSS);

    if(gnss_ret == GNSS_SUCCESS) {
        refresh_display();
    } else {
        switch(gnss_ret) {
            case GNSS_ERR_GPRMC:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "GNSS_ERR_GPRMC"
                );
                break;

            case GNSS_ERR_GPGGA:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "GNSS_ERR_GPGGA"
                );
                break;

            case GNSS_ERR_GPGLL:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "GNSS_ERR_GPGLL"
                );
                break;

            case GNSS_ERR_GPGSV:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "GNSS_ERR_GPGSV"
                );
                break;

            case GNSS_ERR_GPVTG:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "GNSS_ERR_GPVTG"
                );
                break;

            case GNSS_ERR_GPGSA:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "GNSS_ERR_GPGSA"
                );
                break;

            case GNSS_ERR_UNKNOWN:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "UNKNOWN"
                );
                break;

            case GNSS_ERR_EMPTY:
                snprintf(
                    IO.lcd_buf0,
                    LCD_BUF_SIZE,
                    "EMPTY"
                );
                break;

            default:
                break;
        }

        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "%s",
            GNSS->buf
        );
        IO.lcd_show_needed = 1;
    }

    GNSS->buf[0] = '\0'; // clear buf after use

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    return next_status;
}

void gps_imu_module_resume(void) {
}

void gps_imu_module_exit(void) {
    destroy_gnss_reader(GNSS);
    GPS_Serial.end();
}

//////////////////////////////////
// Local functions:
static void refresh_display(void) {
    char floatbuf0[LCD_BUF_SIZE];
    char mode;

    if(GNSS == NULL) {
        return;
    }

    float2str(GNSS->lat, floatbuf0, LCD_BUF_SIZE, 8);
    snprintf(
        IO.lcd_buf0,
        LCD_BUF_SIZE,
        "lat:%s",
        floatbuf0
    );

    float2str(GNSS->lon, floatbuf0, LCD_BUF_SIZE, 8);
    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "lon:%s",
        floatbuf0
    );

    switch(GNSS->mode) {
        case M_UNKNOWN:
            mode = 'x';
            break;

        case M_AUTO:
            mode = 'a';
            break;

        case M_DIFF:
            mode = 'd';
            break;

        default:
            mode = 'e';
            break;
    }

    float2str(GNSS->alt, floatbuf0, LCD_BUF_SIZE, 1);
    snprintf(
        IO.lcd_buf2,
        LCD_BUF_SIZE,
        "mode:%c alt:%s",
        mode,
        floatbuf0
    );

    IO.lcd_show_needed = 1;
}