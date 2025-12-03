#include "bananakit.h"
#ifdef ENABLE_GPS_MODULE

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <Arduino.h>
// #include <SD.h>
// #include <SPI.h>
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "banana_string.h"
#include "bananakit_misc.h"
#include "gps_module.h"
#include "gnss_reader.h"

static void refresh_gps_info_display(void);

#define FLOAT_BUF_SZ 16
#define MAX_MODES 2
#define SQUARE(x) (x*x)
extern callstack_t Callstack;
extern bananakit_io_t IO;
gnss_reader_t *GNSS;

void gps_module_init(void) {
    GNSS = create_gnss_reader();
    Serial.begin(9600);
}

node_status_t gps_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    char c;

    if(GNSS != NULL) {
        while(Serial.available()) {
            gnss_update(GNSS, Serial.read());
        }
    }
    refresh_gps_info_display();
    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        case PIC_PLAY:
            // Start record:
            break;

        case PIC_EQ:
            // Set origin:
            set_origin(GNSS);
            break;

        default:
            break;
    }
    delay(100);
    return next_status;
}

void gps_module_resume(void) {
}

void gps_module_exit(void) {
    destroy_gnss_reader(GNSS);
    GNSS = NULL;
    Serial.end();
}

//////////////////////////////////
// Local functions:
static void refresh_gps_info_display(void) {
    int offset;
    char floatbuf[FLOAT_BUF_SZ];
    double float_value;
    char tail;
    double x, y;

    IO.lcd_clear_callback();

    if(GNSS == NULL) {
        strncpy(IO.lcd_buf, "gps init err", LCD_BUF_SIZE);
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return;
    }

    // Line0: "123.4567,-123.4567"
    // each coordinate keep max 4 digit after float point
    // If !data_initialized then show "gps n/a"
    // If !data_valid then show "-123.4567,-123.4567?"
    if(GNSS->status.data_valid) {
        tail = ' ';
    } else {
        tail = '?';
    }

    if(!GNSS->status.data_initialized) {
        strncpy(IO.lcd_buf, "gps n/a", LCD_BUF_SIZE);
    } else {
        offset = 0;
        float2str(
            ((double) GNSS->lat.degree * GNSS->lat.sign) + GNSS->lat.minute / 60.0,
            floatbuf,
            FLOAT_BUF_SZ,
            4
        );
        offset += snprintf(IO.lcd_buf+offset, LCD_BUF_SIZE-offset, "%s,", floatbuf);
        float2str(
            ((double) GNSS->lon.degree * GNSS->lon.sign) + GNSS->lon.minute / 60.0,
            floatbuf,
            FLOAT_BUF_SZ,
            4
        );
        offset += snprintf(IO.lcd_buf+offset, LCD_BUF_SIZE-offset, "%s%c", floatbuf, tail);
    }
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    // Line1: "2025-12-02 18:23:00"
    // If !data_initialized then show "time n/a"
    // If !data_valid then show "2025-12-02 18:23:00?"
    if(!GNSS->status.data_initialized) {
        strncpy(IO.lcd_buf, "time n/a", LCD_BUF_SIZE);
    } else {
        offset = 0;
        offset += snprintf(IO.lcd_buf+offset, LCD_BUF_SIZE-offset, "%d-%d-%d %d-%d-%d%c",
            GNSS->year, GNSS->month, GNSS->day,
            GNSS->utc_hour, GNSS->utc_min, (int) GNSS->utc_sec,
            tail
        );
    }
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    // Line2: "x:107.12,y:-512.33"
    // If abs(x) > 100 || abs(y) > 100 then show "x,y oversized"
    // If (origin not set) || (!data_initialized) then show "x,y n/a"
    if((!GNSS->status.data_initialized) || (!GNSS->status.origin_initialized)) {
        strncpy(IO.lcd_buf, "x,y n/a", LCD_BUF_SIZE);
    } else {
        gps2localxy(GNSS, &x, &y);
        if(fabs(x) > 100 || fabs(y > 100)) {
            strncpy(IO.lcd_buf, "x,y oversized", LCD_BUF_SIZE);
        } else {
            offset = 0;
            float2str(x, floatbuf, FLOAT_BUF_SZ, 2);
            offset += snprintf(IO.lcd_buf+offset, LCD_BUF_SIZE-offset, "x:%s,", floatbuf);
            float2str(y, floatbuf, FLOAT_BUF_SZ, 2);
            offset += snprintf(IO.lcd_buf+offset, LCD_BUF_SIZE-offset, "y:%s", floatbuf);
        }
    }
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();

    // Line3: "rec:gps003.csv debug:10"
    // If not recording then "rec:off debug:10"
    offset = 0;
    offset += snprintf(
        IO.lcd_buf+offset,
        LCD_BUF_SIZE-offset,
        "rec:off debug:%d",
        GNSS->debug_code
    );
    IO.flags = LCD_REFRESH_LINE3;
    IO.lcd_refresh_callback();
}
#endif