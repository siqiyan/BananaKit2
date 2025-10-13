#include <stdlib.h>
#include <stdint.h>
#include <util/atomic.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"
#include "rc_station.h"


extern callstack_t Callstack;
extern bananakit_io_t IO;

RF24 RF_radio(NRF24_CE_PIN, NRF24_CSN_PIN);
rc_station_t *Station;


void rc_station_interrupt(void);
void init_a2o_frame(a2o_frame_t *frame);
void init_o2a_frame(o2a_frame_t *frame);
void init_v2b_frame(v2b_frame_t *frame);
void init_b2v_frame(b2v_frame_t *frame);
void init_a2l_frame(a2l_frame_t *frame);
void init_l2a_frame(l2a_frame_t *frame);
rc_station_t *init_rc_station(void);
int destroy_rc_station(rc_station_t *station);


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
    l2a_frame_t l2aframe;
    a2l_frame_t a2lframe;
    v2b_frame_t v2bframe;
    b2v_frame_t b2vframe;
    float analog_value_f;
    float speed_multi;
    float scaled_linear_speed;
    float scaled_angular_speed;
    float latitude, longitude, altitude, orientation;

    // Compute speed limit values:
    speed_multi = ((float) analogRead(VOLT0_READ_PIN)) / 1024.0;
    scaled_linear_speed = Station->max_linear_speed * speed_multi;
    scaled_angular_speed = Station->max_angular_speed * speed_multi;

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

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Handle laptop side communication:
    if(Serial.available() > 0) {
        // Receive l2a frame from base station's laptop:
        if(Serial.readBytes((char *) &l2aframe, sizeof(l2a_frame_t)) == sizeof(l2a_frame_t)) {

            // Verify checksum:
            if(compute_checksum((char *) &l2aframe, sizeof(l2a_frame_t)) == l2aframe.checksum) {

                // Timestamp synchronization with laptop:
                if(Station->toggle_ts_sync || (!Station->sync_with_laptop)) {
                    ATOMIC_BLOCK(ATOMIC_FORCEON) {
                        Station->timestamp = l2aframe.timestamp; 
                    }
                    Station->toggle_ts_sync = 0;
                }

                if(abs(Station->timestamp - l2aframe.timestamp) < 1000) {
                    Station->sync_with_laptop = 1;
                    Station->sequence_id = l2aframe.sequence_id;
                    Station->goal_latitude_int = l2aframe.goal_latitude_int;
                    Station->goal_longitude_int = l2aframe.goal_longitude_int;
                    Station->goal_orientation_int = l2aframe.goal_orientation_int;
                    Station->cmd_toggle = l2aframe.cmd_toggle;
                } else {
                    // If incoming data is out-of-date:
                    Station->sync_with_laptop = 0;
                }

                // Reply a2l frame to laptop:
                a2lframe.timestamp = Station->timestamp;
                a2lframe.sequence_id = Station->sequence_id;
                a2lframe.adc_value = Station->adc_value;
                a2lframe.latitude_int = Station->latitude_int;
                a2lframe.longitude_int = Station->longitude_int;
                a2lframe.orientation_int = Station->orientation_int;
                a2lframe.sm_state = Station->sm_state;

                a2lframe.checksum = compute_checksum(
                    (char *) &a2lframe, sizeof(a2l_frame_t)
                );
                Serial.write((char *) &a2lframe, sizeof(a2l_frame_t));
                Serial.flush();

            } // if(compute_checksum())
        } // if(Serial.readBytes())
    } // if(Serial.available())

    //////////////////////////////////////////////////////////////////////////////////////////////
    // Handle vehicle side communication:
    if(RF_radio.available()) {

        // Receive v2b frame from vehicle Arduino:
        RF_radio.read(&v2bframe, sizeof(v2b_frame_t));

        if(abs(Station->timestamp - v2bframe.timestamp) < 1000) {
            Station->adc_value = v2bframe.adc_value;
            Station->latitude_int = v2bframe.latitude_int;
            Station->longitude_int = v2bframe.longitude_int;
            Station->orientation_int = v2bframe.orientation_int;

            // Restore GPS coordinate from int to float:
            latitude = Station->latitude_int / GPS_F2I_MULTI;
            longitude = Station->longitude_int / GPS_F2I_MULTI;
            altitude = Station->altitude_int / GPS_F2I_MULTI;
            orientation = Station->orientation_int / GPS_F2I_MULTI;

            Station->sm_state = v2bframe.sm_state;
            Station->sync_with_vehicle = 1;
        } else {
            // If incoming data is out-of-date:
            Station->sync_with_vehicle = 0;
        }
    } else {
        delay(5);
    }

    // Update b2v frame with the latest data from base station's laptop:
    b2vframe.timestamp = Station->timestamp;
    b2vframe.sequence_id = Station->sequence_id;
    b2vframe.twist_x = Station->twist_x;
    b2vframe.twist_y = Station->twist_y;
    b2vframe.twist_yaw = Station->twist_yaw;
    b2vframe.goal_latitude_int = Station->goal_latitude_int;
    b2vframe.goal_longitude_int = Station->goal_longitude_int;
    b2vframe.goal_orientation_int = Station->goal_orientation_int;
    b2vframe.cmd_toggle = Station->cmd_toggle;

    // Send b2v frame to vehicle Arduino:
    RF_radio.stopListening();
    RF_radio.write(&b2vframe, sizeof(b2v_frame_t));
    RF_radio.startListening();

    //////////////////////////////////////////////////////////////////////////////////////////////

    Station->frame_counter++;

    // Refresh LCD display:
    snprintf(
        IO.lcd_buf0,
        LCD_BUF_SIZE,
        "Laptop:%d Vehicle:%d",
        Station->sync_with_laptop,
        Station->sync_with_vehicle
    );
    if(Station->sync_with_vehicle) {
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "Batt:"
        );
    }
    
    IO.lcd_show_needed = 1;

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

    return next_state;
}

void rc_station_resume(void) {
}

void rc_station_exit(void) {
    destroy_rc_station(Station);
    Station = NULL;
    // TODO: destroy nrf24 object
    IO.interrupt_callback = NULL;
}

void rc_station_interrupt(void) {
    if(Station != NULL) {
        Station->timestamp++;
    }
}


///////////////////////////////////////////////////////////////////
// Local functions:
void init_a2o_frame(a2o_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->adc_value = 0;
    frame->twist_x = 0;
    frame->twist_y = 0;
    frame->twist_yaw = 0;
    frame->goal_latitude_int = 0;
    frame->goal_longitude_int = 0;
    frame->goal_orientation_int = 0;
    frame->cmd_toggle = 0;
    frame->checksum = 0;
}

void init_o2a_frame(o2a_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->latitude_int = 0;
    frame->longitude_int = 0;
    frame->orientation_int = 0;
    frame->sm_state = 0;
    frame->checksum = 0;
}

void init_v2b_frame(v2b_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->latitude_int = 0;
    frame->longitude_int = 0;
    frame->orientation_int = 0;
    frame->adc_value = 0;
    frame->sm_state = 0;
}

void init_b2v_frame(b2v_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->twist_x = 0;
    frame->twist_y = 0;
    frame->twist_yaw = 0;
    frame->goal_latitude_int = 0;
    frame->goal_longitude_int = 0;
    frame->goal_orientation_int = 0;
    frame->cmd_toggle = 0;
}

void init_a2l_frame(a2l_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->adc_value = 0;
    frame->latitude_int = 0;
    frame->longitude_int = 0;
    frame->orientation_int = 0;
    frame->sm_state = 0;
    frame->checksum = 0;
}

void init_l2a_frame(l2a_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->twist_x = 0;
    frame->twist_y = 0;
    frame->twist_yaw = 0;
    frame->goal_latitude_int = 0;
    frame->goal_longitude_int = 0;
    frame->goal_orientation_int = 0;
    frame->cmd_toggle = 0;
    frame->checksum = 0;
}

rc_station_t *init_rc_station(void) {
    rc_station_t *station;
    station = (rc_station_t *) malloc(sizeof(rc_station_t));
    if(station == NULL) {
        return NULL;
    }

    // TODO: init station variables:
    station->joy_neutral_pos_x = 635;
    station->joy_neutral_pos_y = 646;

    return station;
}

int destroy_rc_station(rc_station_t *station) {
    if(station == NULL) {
        return RC_ERR_NUL_PTR;
    }

    free(station);
    return RC_SUCCESS;
}