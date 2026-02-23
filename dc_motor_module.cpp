#include "bananakit.h"
#ifdef ENABLE_DC_MOTOR_MODULE

#include <Arduino.h>
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "bananakit_misc.h"
#include "banana_string.h"
#include "dc_motor_module.h"
extern callstack_t Callstack;
extern bananakit_io_t IO;

#define ADC_R1 10250.0
#define ADC_R2 46400.0

static int32_t currTimestamp;
static int32_t prevTimestamp;

float get_adc_voltage(int16_t adc_value, float ref_voltage, float r1, float r2) {
    return ((float) adc_value) * ref_voltage / 1024.0 * (r1 + r2) / r2;
}

void dc_motor_init(void) {
    pinMode(DC_MOTOR_EN, OUTPUT);
    digitalWrite(DC_MOTOR_EN, LOW);
    prevTimestamp = millis();
}

node_status_t dc_motor_update(void) {
    node_status_t next_state = NODE_RUNNING;
    int16_t motor_adc = analogRead(DC_MOTOR_ADC);
    int16_t motor_cmd_adc = analogRead(VOLT0_READ_PIN);
    int16_t motor_pwm = map(motor_cmd_adc, 0, 1023, 0, 255);

    currTimestamp = millis();

    analogWrite(DC_MOTOR_EN, motor_pwm);

    if(currTimestamp - prevTimestamp > 1000) {
        IO.lcd_clear_callback();
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "ADC:%d",
            motor_adc
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();

        float2str(
            //get_adc_voltage(motor_adc, 5.0, ADC_R1, ADC_R2),
            get_adc_voltage(motor_adc, 5.0, 47000.0, 10000.0),
            IO.lcd_buf,
            LCD_BUF_SIZE,
            2
        );
        IO.flags = LCD_REFRESH_LINE1;
        IO.lcd_refresh_callback();

        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "SPD:%d",
            motor_pwm
        );
        IO.flags = LCD_REFRESH_LINE2;
        IO.lcd_refresh_callback();
        prevTimestamp = currTimestamp;
    }

    switch(IO.keypress) {
        case PIC_POWER:
            next_state = NODE_EXITING;
            break;

        default:
            break;
    }

    return next_state;
}

void dc_motor_resume(void) {
}

void dc_motor_exit(void) {
}

#endif
