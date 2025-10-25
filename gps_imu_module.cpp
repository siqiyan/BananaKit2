#include <string.h>
#include <stdlib.h>
#include <SoftwareSerial.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
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

    Serial.begin(9600);
}

node_status_t gps_imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    int gnss_ret;
    char c;
    uint8_t nmea_start;

    // IO.lcd_clear_callback();

    if(GNSS == NULL) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "GNSS init err"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return NODE_EXITING;
    }

    // if(GPS_Serial.available()) {
    //     char c = GPS_Serial.read();
    //     Serial.write(c);
    // }

    // snprintf(
    //     IO.lcd_buf,
    //     LCD_BUF_SIZE,
    //     "wait gps reading"
    // );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();
    while(!GPS_Serial.available()) {
        delay(50);
    }

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
    GNSS->buf[GNSS->buf_count] = '\0'; // set nul character

    char field_buf[FIELD_BUF_SZ];
    char floatbuf[LCD_BUF_SIZE];
    // char floatbuf[16];
    char hemisphere_lat;
    char hemisphere_lon;
    int field_sz;
    int atof_ret;
    double latlon;
    IO.lcd_clear_callback();
    if(GNSS->buf_count >= 6) {
        if(strncmp(GNSS->buf, "$GPRMC", 6) == 0) {

            // Hemisphere of latitude
            field_sz = field_extract(GNSS->buf, GNSS->buf_count, ',', 4, field_buf, FIELD_BUF_SZ);
            if((field_sz) > 0) {
                hemisphere_lat = field_buf[0];
            } else {
                hemisphere_lat = '!';
            }

            // Hemisphere of longitude
            field_sz = field_extract(GNSS->buf, GNSS->buf_count, ',', 6, field_buf, FIELD_BUF_SZ);
            if(field_sz > 0) {
                hemisphere_lon = field_buf[0];
            } else {
                hemisphere_lon = '!';
            }

            // Latitude: 3 DDMM.MMMMMMM
            field_sz = field_extract(GNSS->buf, GNSS->buf_count, ',', 3, field_buf, FIELD_BUF_SZ);
            if(field_sz >= 7) {
                // snprintf(
                //     IO.lcd_buf,
                //     LCD_BUF_SIZE,
                //     "lat:\"%s\"",
                //     field_buf
                // );
                // IO.flags = LCD_REFRESH_LINE0;
                // IO.lcd_refresh_callback();

                atof_ret = gps_atof(field_buf, field_sz, 2, &latlon);
                if(atof_ret != 1) {
                    snprintf(
                        IO.lcd_buf,
                        LCD_BUF_SIZE,
                        "lat:float err%d",
                        atof_ret
                    );
                } else {
                    float2str(latlon, floatbuf, LCD_BUF_SIZE, 8);
                    snprintf(
                        IO.lcd_buf,
                        LCD_BUF_SIZE,
                        "lat:%c%s",
                        hemisphere_lat,
                        floatbuf
                    );
                    Serial.print("lat:");
                    Serial.println(latlon);
                }
            } else {
                snprintf(
                    IO.lcd_buf,
                    LCD_BUF_SIZE,
                    "field_sz:%d",
                    field_sz
                );
            }
            IO.flags = LCD_REFRESH_LINE0;
            IO.lcd_refresh_callback();

            // Longitude: 5 DDDMM.MMMMMMM
            field_sz = field_extract(GNSS->buf, GNSS->buf_count, ',', 5, field_buf, FIELD_BUF_SZ);
            if(field_sz >= 8) {
                // snprintf(
                //     IO.lcd_buf,
                //     LCD_BUF_SIZE,
                //     "lon:\"%s\"",
                //     field_buf
                // );
                // IO.flags = LCD_REFRESH_LINE1;
                // IO.lcd_refresh_callback();

                atof_ret = gps_atof(field_buf, field_sz, 3, &latlon);
                if(atof_ret != 1) {
                    snprintf(
                        IO.lcd_buf,
                        LCD_BUF_SIZE,
                        "lon:float err%d",
                        atof_ret
                    );
                } else {
                    float2str(latlon, floatbuf, LCD_BUF_SIZE, 8);
                    snprintf(
                        IO.lcd_buf,
                        LCD_BUF_SIZE,
                        "lon:%c%s",
                        hemisphere_lon,
                        floatbuf
                    );
                    Serial.print("lon:");
                    Serial.println(latlon);
                }
                
            } else {
                snprintf(
                    IO.lcd_buf,
                    LCD_BUF_SIZE,
                    "field_sz:%d",
                    field_sz
                );
            }
            IO.flags = LCD_REFRESH_LINE1;
            IO.lcd_refresh_callback();

            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "GPRMC"
            );
            IO.flags = LCD_REFRESH_LINE2;
            IO.lcd_refresh_callback();
        }
        
        // else if(strncmp(GNSS->buf, "$GPGGA", 6) == 0) {

        //     // Latitude: 3 DDMM.MMMMMMM
        //     if((field_sz_ret = field_extract(GNSS->buf, GNSS->buf_count, ',', 2, field_buf, FIELD_BUF_SZ)) >= 7) {
        //         snprintf(
        //             IO.lcd_buf,
        //             LCD_BUF_SIZE,
        //             "lat:%s",
        //             field_buf
        //         );
        //     } else {
        //         snprintf(
        //             IO.lcd_buf,
        //             LCD_BUF_SIZE,
        //             "ret:%d",
        //             field_sz_ret
        //         );
        //     }
        //     IO.flags = LCD_REFRESH_LINE0;
        //     IO.lcd_refresh_callback();

        //     // Longitude: 4 DDDMM.MMMMMMM
        //     if((field_sz_ret = field_extract(GNSS->buf, GNSS->buf_count, ',', 4, field_buf, FIELD_BUF_SZ)) >= 8) {
        //         snprintf(
        //             IO.lcd_buf,
        //             LCD_BUF_SIZE,
        //             "lon:%s",
        //             field_buf
        //         );
        //     } else {
        //         snprintf(
        //             IO.lcd_buf,
        //             LCD_BUF_SIZE,
        //             "ret:%d",
        //             field_sz_ret
        //         );
        //     }
        //     IO.flags = LCD_REFRESH_LINE1;
        //     IO.lcd_refresh_callback();

        //     snprintf(
        //         IO.lcd_buf,
        //         LCD_BUF_SIZE,
        //         "GPGGA"
        //     );
        //     IO.flags = LCD_REFRESH_LINE2;
        //     IO.lcd_refresh_callback();
        // }
        
    } else {
        // snprintf(IO.lcd_buf, LCD_BUF_SIZE, "skip short buf");
        // IO.flags = LCD_REFRESH_LINE0;
        // IO.lcd_refresh_callback();

        // snprintf(IO.lcd_buf, LCD_BUF_SIZE, "buf_count:%d", GNSS->buf_count);
        // IO.flags = LCD_REFRESH_LINE1;
        // IO.lcd_refresh_callback();
    }

    // gnss_ret = parse_gnss_data_buf(GNSS);
    // if(gnss_ret == GNSS_SUCCESS) {
    //     refresh_display();
    // }


    // else {
    //     switch(gnss_ret) {
    //         case GNSS_ERR_GPRMC:
    //             snprintf(
    //                 IO.lcd_buf0,
    //                 LCD_BUF_SIZE,
    //                 "GNSS_ERR_GPRMC"
    //             );
    //             break;

    //         case GNSS_ERR_GPGGA:
    //             snprintf(
    //                 IO.lcd_buf0,
    //                 LCD_BUF_SIZE,
    //                 "GNSS_ERR_GPGGA"
    //             );
    //             break;

    //         // case GNSS_ERR_GPGLL:
    //         //     snprintf(
    //         //         IO.lcd_buf0,
    //         //         LCD_BUF_SIZE,
    //         //         "GNSS_ERR_GPGLL"
    //         //     );
    //         //     break;

    //         // case GNSS_ERR_GPGSV:
    //         //     snprintf(
    //         //         IO.lcd_buf0,
    //         //         LCD_BUF_SIZE,
    //         //         "GNSS_ERR_GPGSV"
    //         //     );
    //         //     break;

    //         // case GNSS_ERR_GPVTG:
    //         //     snprintf(
    //         //         IO.lcd_buf0,
    //         //         LCD_BUF_SIZE,
    //         //         "GNSS_ERR_GPVTG"
    //         //     );
    //         //     break;

    //         // case GNSS_ERR_GPGSA:
    //         //     snprintf(
    //         //         IO.lcd_buf0,
    //         //         LCD_BUF_SIZE,
    //         //         "GNSS_ERR_GPGSA"
    //         //     );
    //         //     break;

    //         // case GNSS_ERR_UNKNOWN:
    //         //     snprintf(
    //         //         IO.lcd_buf0,
    //         //         LCD_BUF_SIZE,
    //         //         "UNKNOWN"
    //         //     );
    //         //     break;

    //         case GNSS_ERR_EMPTY:
    //             snprintf(
    //                 IO.lcd_buf0,
    //                 LCD_BUF_SIZE,
    //                 "EMPTY"
    //             );
    //             break;

    //         default:
    //             break;
    //     }

    //     snprintf(
    //         IO.lcd_buf1,
    //         LCD_BUF_SIZE,
    //         "%s",
    //         GNSS->buf
    //     );
    //     IO.lcd_show_needed = 1;
    // }

    GNSS->buf[0] = '\0'; // clear buf after use

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    delay(1000);

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
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "lat:%s",
        floatbuf0
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    float2str(GNSS->lon, floatbuf0, LCD_BUF_SIZE, 8);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "lon:%s",
        floatbuf0
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

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
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "mode:%c alt:%s",
        mode,
        floatbuf0
    );
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();
}