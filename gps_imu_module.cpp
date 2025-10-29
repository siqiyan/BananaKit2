#include <string.h>
#include <stdlib.h>
#include <MPU6050.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "bananakit_misc.h"
#include "gps_imu_module.h"
#include "gnss_reader.h"

#define FLOAT_BUF_SZ 16

extern callstack_t Callstack;
extern bananakit_io_t IO;

gnss_reader_t *GNSS;
MPU6050 IMU;
uint8_t IMU_ok;

#define MAX_PAGES 3
int8_t Page;


static void refresh_gps_display(void);
static void refresh_imu_display(void);


void gps_imu_module_init(void) {
    GNSS = create_gnss_reader();
    Serial.begin(9600);

    IMU.initialize();
    if(IMU.testConnection() == false) {
        IMU_ok = 0;
    } else {
        IMU_ok = 1;
        IMU.setXAccelOffset(0); //Set your accelerometer offset for axis X
        IMU.setYAccelOffset(0); //Set your accelerometer offset for axis Y
        IMU.setZAccelOffset(0); //Set your accelerometer offset for axis Z
        IMU.setXGyroOffset(0);  //Set your gyro offset for axis X
        IMU.setYGyroOffset(0);  //Set your gyro offset for axis Y
        IMU.setZGyroOffset(0);  //Set your gyro offset for axis Z
    }

    Page = 0;
}

node_status_t gps_imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    char c;
    uint8_t nmea_start;
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    // IO.lcd_clear_callback();

    if(IMU_ok) {
        IMU.getMotion6(
            &ax, &ay, &az,
            &gx, &gy, &gz
        );
    }

    if(GNSS != NULL) {
        GNSS->buf_count = 0;
        nmea_start = 0;
        while(Serial.available() && GNSS->buf_count < GNSS_UART_BUF_SZ - 1) {
            c = Serial.read();
            if(c == '$') {
                nmea_start = 1;
            }
            if(!nmea_start) {
                continue;
            }
            GNSS->buf[GNSS->buf_count++] = c;
        }
        GNSS->buf[GNSS->buf_count] = '\0'; // set nul character

        GNSS->debug_code0 = 0;
        GNSS->debug_code1 = 0;
        GNSS->debug_code2 = 0;
        GNSS->debug_code3 = 0;

        parse_gnss_data_buf(GNSS);
        GNSS->buf[0] = '\0'; // clear buf after use
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
    // IO.flags = LCD_REFRESH_LINE0;
    // IO.lcd_refresh_callback();
    // while(!GPS_Serial.available()) {
    //     delay(50);
    // }

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

    delay(50);

    return next_status;
}

void gps_imu_module_resume(void) {
}

void gps_imu_module_exit(void) {
    destroy_gnss_reader(GNSS);
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
        "%d %d %d %d",
        GNSS->debug_code0,
        GNSS->debug_code1,
        GNSS->debug_code2,
        GNSS->debug_code3
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    if(GNSS->data_valid) {
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
}

static void refresh_imu_display(void) {
    IO.lcd_clear_callback();
    if(!IMU_ok) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "IMU init fail"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return;
    }

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "%d %d %d",
        ax, ay, az
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "%d %d %d",
        gx, gy, gz
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();
}