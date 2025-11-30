#include <string.h>
#include <stdlib.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "banana_string.h"
#include "bananakit_misc.h"
#include "imu_module.h"
#include "imu_reader.h"

#define FLOAT_BUF_SZ 16
#define MAX_PAGES 1
extern callstack_t Callstack;
extern bananakit_io_t IO;
imu_reader_t *IMU;
int8_t Page;
static void refresh_imu_display(void);

void imu_module_init(void) {
    IMU = create_imu_reader();
    Page = 0;
}

node_status_t imu_module_update(void) {
    node_status_t next_status = NODE_RUNNING;
    if(IMU != NULL && IMU->dmp_ready) {
        imu_update(IMU);
    }
    if(Page == 0) {
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

void imu_module_resume(void) {
}

void imu_module_exit(void) {
    destroy_imu_reader(IMU);
    IMU = NULL;
}

//////////////////////////////////
// Local functions:
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