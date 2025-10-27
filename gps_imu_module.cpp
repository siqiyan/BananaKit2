#include <string.h>
#include <stdlib.h>
#include <SoftwareSerial.h>
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

// SoftwareSerial GPS_Serial(GT_U7_TX, GT_U7_RX);
gnss_reader_t *GNSS;
MPU6050 IMU;
uint8_t IMU_ok;
uint8_t Page;


static void refresh_display(void);


void gps_imu_module_init(void) {
    pinMode(GT_U7_TX, INPUT);
    pinMode(GT_U7_RX, OUTPUT);

    GNSS = create_gnss_reader();
    // GPS_Serial.begin(9600);
    // GPS_Serial.listen();
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

    Serial.begin(9600);
}

node_status_t gps_imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    int gnss_ret;
    char c;
    uint8_t nmea_start;
    char floatbuf[FLOAT_BUF_SZ];
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    // IO.lcd_clear_callback();

    if(!IMU_ok) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "IMU init fail"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return next_status;
    }

    if(GNSS == NULL) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "GNSS init err"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return next_status;
    }

    IMU.getMotion6(
        &ax, &ay, &az,
        &gx, &gy, &gz
    );

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
    // while(!GPS_Serial.available()) {
    //     delay(50);
    // }

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

    IO.lcd_clear_callback();
    gnss_ret = parse_gnss_data_buf(GNSS);

    if(Page == 0) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "ret:%d d1:%d d2:%d",
            gnss_ret,
            GNSS->debug_code,
            GNSS->debug_code2
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
    } else if(Page == 1) {
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

    GNSS->buf[0] = '\0'; // clear buf after use

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        case PIC_0:
            Page = 0;
            break;

        case PIC_1:
            Page = 1;
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
    Serial.end();
}

//////////////////////////////////
// Local functions:
static void refresh_display(void) {
    // char floatbuf0[LCD_BUF_SIZE];
    // char mode;

    // if(GNSS == NULL) {
    //     return;
    // }

    // float2str(GNSS->lat, floatbuf0, LCD_BUF_SIZE, 8);
    // snprintf(
    //     IO.lcd_buf,
    //     LCD_BUF_SIZE,
    //     "lat:%s",
    //     floatbuf0
    // );
    // IO.flags = LCD_REFRESH_LINE0;
    // IO.lcd_refresh_callback();

    // float2str(GNSS->lon, floatbuf0, LCD_BUF_SIZE, 8);
    // snprintf(
    //     IO.lcd_buf,
    //     LCD_BUF_SIZE,
    //     "lon:%s",
    //     floatbuf0
    // );
    // IO.flags = LCD_REFRESH_LINE1;
    // IO.lcd_refresh_callback();

    // switch(GNSS->mode) {
    //     case M_UNKNOWN:
    //         mode = 'x';
    //         break;

    //     case M_AUTO:
    //         mode = 'a';
    //         break;

    //     case M_DIFF:
    //         mode = 'd';
    //         break;

    //     default:
    //         mode = 'e';
    //         break;
    // }

    // float2str(GNSS->alt, floatbuf0, LCD_BUF_SIZE, 1);
    // snprintf(
    //     IO.lcd_buf,
    //     LCD_BUF_SIZE,
    //     "mode:%c alt:%s",
    //     mode,
    //     floatbuf0
    // );
    // IO.flags = LCD_REFRESH_LINE2;
    // IO.lcd_refresh_callback();
}