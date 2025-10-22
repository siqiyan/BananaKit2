#include <stdio.h>
#include <stdint.h>

#include "joystick_module.h"
#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "bananakit_misc.h"

extern callstack_t Callstack;
extern bananakit_io_t IO;

void joystick_init(void) {
    pinMode(JOY_SW, INPUT_PULLUP);
    pinMode(JOY_PUSH_BUTTON_0, INPUT_PULLUP);
    pinMode(JOY_PUSH_BUTTON_1, INPUT_PULLUP);
}

node_status_t joystick_update(void) {
    int joy_axis_x;
    int joy_axis_y;
    int volt0;
    uint8_t joy_sw;
    uint8_t joy_button_0;
    uint8_t joy_button_1;
    node_status_t next_status = NODE_RUNNING;

    joy_axis_x = analogRead(JOY_VRX);
    joy_axis_y = analogRead(JOY_VRY);
    joy_sw = digitalRead(JOY_SW);
    joy_button_0 = digitalRead(JOY_PUSH_BUTTON_0);
    joy_button_1 = digitalRead(JOY_PUSH_BUTTON_1);
    volt0 = analogRead(VOLT0_READ_PIN);

    IO.lcd_clear_callback();

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "x:%d y:%d",
        joy_axis_x,
        joy_axis_y
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "sw:%d v0:%d",
        joy_sw,
        volt0
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "b0:%d b1:%d",
        joy_button_0,
        joy_button_1
    );
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();

    switch(IO.keypress) {
        case PIC_4:
            // Return to previous menu:
            next_status = NODE_EXITING;
            break;
        
        default:
            break;
    }

    delay(50);

    return next_status;
}

void joystick_resume(void) {

}

void joystick_exit(void) {

}