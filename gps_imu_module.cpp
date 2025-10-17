#include <string.h>
#include <SoftwareSerial.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"
#include "gps_imu_module.h"


extern callstack_t Callstack;
extern bananakit_io_t IO;

SoftwareSerial GPS_Serial(GT_U7_TX, GT_U7_RX);


void gps_imu_module_init(void) {
    pinMode(GT_U7_TX, INPUT);
    pinMode(GT_U7_RX, OUTPUT);

    GPS_Serial.begin(9600);
    GPS_Serial.listen();
}

node_status_t gps_imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    int count = 0;
    int i;

    while(GPS_Serial.available() && count < LCD_BUF_SIZE - 1) {
        IO.lcd_buf0[count] = GPS_Serial.read();
        count++;
    }

    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "count:%d",
        count
    );

    IO.lcd_buf0[LCD_BUF_SIZE - 1] = '\0';

    IO.lcd_show_needed = 1;

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }
    return NODE_RUNNING;
}

void gps_imu_module_resume(void) {

}

void gps_imu_module_exit(void) {

}