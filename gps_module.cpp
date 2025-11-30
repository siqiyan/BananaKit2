#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "banana_string.h"
#include "bananakit_misc.h"
#include "gps_module.h"
#include "gnss_reader.h"

#define FLOAT_BUF_SZ 16
// #define MAX_PAGES 1
extern callstack_t Callstack;
extern bananakit_io_t IO;
gnss_reader_t *GNSS;
// int8_t Page;
static void refresh_gps_display(void);

void gps_module_init(void) {
    GNSS = create_gnss_reader();
    Serial.begin(9600);
    // Page = 0;
}

node_status_t gps_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    char c;

    if(GNSS != NULL) {
        while(Serial.available()) {
            gnss_update(GNSS, Serial.read());
        }
    }
    refresh_gps_display();
    // if(Page == 0) {
    //     refresh_gps_display();
    // }
    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        case PIC_2:
            // Page up:
            // Page = max(0, Page - 1);
            break;

        case PIC_8:
            // Page down:
            // Page = min(MAX_PAGES - 1, Page + 1);
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
static void refresh_gps_display(void) {
    char floatbuf[FLOAT_BUF_SZ];

    IO.lcd_clear_callback();

    if(GNSS == NULL) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "GNSS init err"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return;
    }

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "%d %d %d %d %d",
        GNSS->debug_code0,
        GNSS->debug_code1,
        GNSS->debug_code2,
        GNSS->debug_code3,
        GNSS->status.data_valid
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    float2str(GNSS->lat.minute, floatbuf, FLOAT_BUF_SZ, 5);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "lat:%d %s%c",
        GNSS->lat.degree,
        floatbuf,
        (GNSS->lat.sign > 0) ? 'N' : 'S'
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    float2str(GNSS->lon.minute, floatbuf, FLOAT_BUF_SZ, 5);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "lon:%d %s%c",
        GNSS->lon.degree,
        floatbuf,
        (GNSS->lon.sign > 0) ? 'E' : 'W'
    );
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "%d %d %d %d",
        GNSS->utc_hour,
        GNSS->utc_min,
        GNSS->year,
        GNSS->month
    );
    IO.flags = LCD_REFRESH_LINE3;
    IO.lcd_refresh_callback();
}