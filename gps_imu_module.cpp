#include <string.h>
#include <stdlib.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "bananakit_misc.h"
#include "gps_imu_module.h"
#include "gnss_reader.h"
#include "imu_reader.h"

#define FLOAT_BUF_SZ 16

extern callstack_t Callstack;
extern bananakit_io_t IO;

gnss_reader_t *GNSS;
imu_reader_t *IMU;

#define MAX_PAGES 2
int8_t Page;


static void refresh_gps_display(void);
static void refresh_imu_display(void);


void gps_imu_module_init(void) {
    GNSS = create_gnss_reader();
    Serial.begin(9600);

    IMU = create_imu_reader();
    
    Page = 0;
}

node_status_t gps_imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    char c;
    uint8_t nmea_start;

    if(GNSS != NULL) {
        while(Serial.available()) {
            gnss_update(GNSS, Serial.read());
        }
    }

    if(IMU != NULL && IMU->dmp_ready) {
        imu_update(IMU);
    }

    if(Page == 0) {
        refresh_gps_display();
    } else if(Page == 1) {
        refresh_imu_display();
    }

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        case PIC_2:
            // Page up:
            Page = max(0, Page - 1);
            break;

        case PIC_8:
            // Page down:
            Page = min(MAX_PAGES - 1, Page + 1);
            break;

        default:
            break;
    }

    delay(100);

    return next_status;
}

void gps_imu_module_resume(void) {
}

void gps_imu_module_exit(void) {
    destroy_gnss_reader(GNSS);
    GNSS = NULL;
    Serial.end();
    destroy_imu_reader(IMU);
    IMU = NULL;
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
        GNSS->data_valid
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    float2str(GNSS->latitude_minute, floatbuf, FLOAT_BUF_SZ, 5);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "lat:%d %s%c",
        GNSS->latitude_degree,
        floatbuf,
        GNSS->latitude_hemisphere
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    float2str(GNSS->longitude_minute, floatbuf, FLOAT_BUF_SZ, 5);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "lon:%d %s%c",
        GNSS->longitude_degree,
        floatbuf,
        GNSS->longitude_hemisphere
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

static void refresh_imu_display(void) {
    char floatbuf[FLOAT_BUF_SZ];

    if(IMU == NULL || (!IMU->dmp_ready)) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "IMU init err"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return;
    }

    IO.lcd_clear_callback();

    float2str(IMU->ypr[0] * RAD_TO_DEG, floatbuf, FLOAT_BUF_SZ, 2);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "yaw:%s",
        floatbuf
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    float2str(IMU->ypr[1] * RAD_TO_DEG, floatbuf, FLOAT_BUF_SZ, 2);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "pitch:%s",
        floatbuf
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    float2str(IMU->ypr[2] * RAD_TO_DEG, floatbuf, FLOAT_BUF_SZ, 2);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "roll:%s",
        floatbuf
    );
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();
}