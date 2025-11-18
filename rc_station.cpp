#include <stdlib.h>
#include <stdint.h>
#include <Arduino.h>
#include <util/atomic.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "bananakit_misc.h"
#include "banana_string.h"
#include "rc_station.h"
#include "rc_vehicle_common.h"


extern callstack_t Callstack;
extern bananakit_io_t IO;

RF24 RF_radio(NRF24_CE_PIN, NRF24_CSN_PIN);
rc_station_t *Station;


void rc_station_interrupt(void);
rc_station_t *init_rc_station(void);
int destroy_rc_station(rc_station_t *station);
void refresh_lcd(void);


void rc_station_init(void) {
    Serial.begin(UART_SPEED_BPS);

    // NRF24 Initialize:
    RF_radio.begin();
    RF_radio.openWritingPipe((const uint8_t *) NRF24_ADDR_BASE);
    RF_radio.openReadingPipe(1, (const uint8_t *) NRF24_ADDR_VEHICLE);
    RF_radio.setPALevel(RF24_PA_HIGH);
    RF_radio.startListening();

    // Init rc station shared variables:
    Station = init_rc_station();

    // Init joystick:
    pinMode(JOY_SW, INPUT_PULLUP);
    pinMode(JOY_PUSH_BUTTON_0, INPUT_PULLUP);
    pinMode(JOY_PUSH_BUTTON_1, INPUT_PULLUP);

    // Attach interrupt handler:
    IO.interrupt_callback = rc_station_interrupt;
}

node_status_t rc_station_update(void) {
    node_status_t next_state = NODE_RUNNING;
    v2s_frame_t v2sframe;
    s2v_frame_t s2vframe;
    s2l_frame_t s2lframe;
    l2s_frame_t l2sframe;
    float analog_value_f;
    float scaled_linear_speed;
    float scaled_angular_speed;
    float latitude, longitude, altitude, orientation;

    init_v2s_frame(&v2sframe);
    init_s2v_frame(&s2vframe);
    init_s2l_frame(&s2lframe);
    init_l2s_frame(&l2sframe);

    // Compute speed limit values:
    Station->speed_multi = ((float) analogRead(VOLT0_READ_PIN)) / 1024.0;
    scaled_linear_speed = MAX_LINEAR_SPEED * Station->speed_multi;
    scaled_angular_speed = MAX_ANGULAR_SPEED * Station->speed_multi;

    // Compute longitudinal twist command:
    // Longitudinal corresponds to joystick x-axis, positive value forward
    analog_value_f = (float) (analogRead(JOY_VRX) - Station->joy_neutral_pos_x);
    if(analog_value_f >= 0) {
        Station->twist_x = analog_value_f / (1024 - Station->joy_neutral_pos_x) * scaled_linear_speed;
    } else {
        Station->twist_x = analog_value_f / Station->joy_neutral_pos_x * scaled_linear_speed;
    }

    // Compute rotation twist command:
    analog_value_f = (float) (analogRead(JOY_VRY) - Station->joy_neutral_pos_y);
    if(analog_value_f >= 0) {
        Station->twist_yaw = analog_value_f / (1024 - Station->joy_neutral_pos_y) * scaled_angular_speed;
    } else {
        Station->twist_yaw = analog_value_f / Station->joy_neutral_pos_y * scaled_angular_speed;
    }

    // E-stop command:
    Station->status_code.estop = 0; // estop off by default
    if(digitalRead(JOY_PUSH_BUTTON_0) == LOW) {
        delay(20); // de-noising
        if(digitalRead(JOY_PUSH_BUTTON_0) == LOW) {
            Station->status_code.estop = 1;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Handle laptop side communication:
    if(Serial.available() > 0) {
        // Receive l2a frame from base station's laptop:
        if(Serial.readBytes((char *) &l2sframe, sizeof(l2s_frame_t)) == sizeof(l2s_frame_t)) {

            // Verify checksum:
            if(compute_checksum((char *) &l2sframe, sizeof(l2s_frame_t)) == l2sframe.checksum) {

                // Timestamp synchronization with laptop:
                if(Station->toggle_ts_sync || (!Station->status_code.sync_with_laptop)) {
                    ATOMIC_BLOCK(ATOMIC_FORCEON) {
                        Station->timestamp = l2sframe.timestamp; 
                    }
                    Station->toggle_ts_sync = 0;
                }

                if(abs(Station->timestamp - l2sframe.timestamp) < 1000) {
                    Station->status_code.sync_with_laptop = 1;

                    // Status code update (selective):
                    Station->status_code.navigate = l2sframe.status_code.navigate;

                    Station->goal_latitude_int = l2sframe.goal_latitude_int;
                    Station->goal_longitude_int = l2sframe.goal_longitude_int;
                    Station->goal_orientation_int = l2sframe.goal_orientation_int;
                } else {
                    // If incoming data is out-of-date:
                    Station->status_code.sync_with_laptop = 0;
                }

                // Reply s2l frame to laptop:
                s2lframe.timestamp = Station->timestamp;
                s2lframe.status_code = Station->status_code;
                s2lframe.sequence_id = Station->frame_counter;
                s2lframe.adc_value = Station->adc_value;
                s2lframe.latitude_int = Station->latitude_int;
                s2lframe.longitude_int = Station->longitude_int;
                s2lframe.orientation_int = Station->orientation_int;
                s2lframe.sm_state = Station->sm_state;

                s2lframe.checksum = compute_checksum(
                    (char *) &s2lframe, sizeof(s2l_frame_t)
                );
                Serial.write((char *) &s2lframe, sizeof(s2l_frame_t));
                Serial.flush();

            } // if(compute_checksum())
        } // if(Serial.readBytes())
    } // if(Serial.available())

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Handle vehicle side communication:
    if(RF_radio.available()) {

        // Receive v2b frame from vehicle Arduino:
        RF_radio.read(&v2sframe, sizeof(v2s_frame_t));

        if(abs(Station->timestamp - v2sframe.timestamp) < 1000) {
            Station->status_code.sync_with_vehicle = 1;

            // Update status code (selective):
            Station->status_code.navigate_reply = v2sframe.status_code.navigate_reply;

            Station->adc_value = v2sframe.adc_value;
            Station->latitude_int = v2sframe.latitude_int;
            Station->longitude_int = v2sframe.longitude_int;
            Station->orientation_int = v2sframe.orientation_int;
            Station->sm_state = v2sframe.sm_state;

        } else {
            // If incoming data is out-of-date:
            Station->status_code.sync_with_vehicle = 0;
        }
    } else {
        delay(5);
    }

    // Reply s2v frame to vehicle:
    s2vframe.timestamp = Station->timestamp;
    s2vframe.status_code = Station->status_code;
    s2vframe.sequence_id = Station->frame_counter;
    // Convert cmd_vel from float to int for transmition:
    s2vframe.twist_x = (int16_t) (Station->twist_x * CMD_VEL_MULTI);
    s2vframe.twist_y = (int16_t) (Station->twist_y * CMD_VEL_MULTI);
    s2vframe.twist_yaw = (int16_t) (Station->twist_yaw * CMD_VEL_MULTI);
    s2vframe.goal_latitude_int = Station->goal_latitude_int;
    s2vframe.goal_longitude_int = Station->goal_longitude_int;
    s2vframe.goal_orientation_int = Station->goal_orientation_int;
    RF_radio.stopListening();
    RF_radio.write(&s2vframe, sizeof(s2v_frame_t));
    RF_radio.startListening();

    //////////////////////////////////////////////////////////////////////////////////////////////

    Station->frame_counter++;
    refresh_lcd();

    switch(IO.keypress) {
        case PIC_4:
            next_state = NODE_EXITING;
            break;

        case PIC_EQ:
            // Calibrate joystick neutral position:
            Station->joy_neutral_pos_x = analogRead(JOY_VRX);
            Station->joy_neutral_pos_y = analogRead(JOY_VRY);
            break;

        case PIC_CHAOS:
            // Calibrate timestamp with laptop:
            Station->toggle_ts_sync = 1;
            break;

        default:
            break;
    }

    delay(50);

    return next_state;
}

void rc_station_resume(void) {
}

void rc_station_exit(void) {
    destroy_rc_station(Station);
    Station = NULL;

    // TODO: turn off nrf24

    IO.interrupt_callback = NULL;
}

void rc_station_interrupt(void) {
    if(Station != NULL) {
        Station->timestamp++;
    }
}


///////////////////////////////////////////////////////////////////
// Local functions:
void refresh_lcd(void) {
    float batt_volt;
    char floatbuf0[LCD_BUF_SIZE];
    char floatbuf1[LCD_BUF_SIZE];
    // char floatbuf2[LCD_BUF_SIZE];

    IO.lcd_clear_callback();

    float2str(Station->speed_multi, floatbuf0, LCD_BUF_SIZE, 2);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "L:%d V:%d S:%s",
        Station->status_code.sync_with_laptop,
        Station->status_code.sync_with_vehicle,
        floatbuf0
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    if(Station->status_code.sync_with_vehicle) {
        batt_volt = compute_batt_volt(Station->adc_value);
        float2str(batt_volt, floatbuf0, LCD_BUF_SIZE, 2);
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "batt:%s sm:%d",
            floatbuf0,
            Station->sm_state
        );
    } else {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "batt:n/a sm:n/a"
        );
    }
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    float2str(Station->twist_x, floatbuf0, LCD_BUF_SIZE, 1);
    float2str(Station->twist_yaw, floatbuf1, LCD_BUF_SIZE, 1);
    
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "vx:%s vz:%s",
        floatbuf0,
        floatbuf1
    );
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();
}

rc_station_t *init_rc_station(void) {
    rc_station_t *station;
    station = (rc_station_t *) malloc(sizeof(rc_station_t));
    if(station == NULL) {
        return NULL;
    }

    station->timestamp = 0;
    station->frame_counter = 0;
    init_status_code(&station->status_code);
    station->twist_x = 0;
    station->twist_y = 0;
    station->twist_yaw = 0;
    station->goal_latitude_int = 0;
    station->goal_longitude_int = 0;
    station->goal_orientation_int = 0;
    station->adc_value = 0;
    station->latitude_int = 0;
    station->longitude_int = 0;
    station->altitude_int = 0;
    station->orientation_int = 0;
    station->sm_state = 0;
    station->toggle_ts_sync = 0;
    station->joy_neutral_pos_x = 635;
    station->joy_neutral_pos_y = 646;
    station->speed_multi = 0.0;
    return station;
}

int destroy_rc_station(rc_station_t *station) {
    if(station == NULL) {
        return RC_ERR_NUL_PTR;
    }

    free(station);
    return RC_SUCCESS;
}