#include "bananakit.h"
#ifdef ENABLE_RC_STATION

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <Arduino.h>
#include <util/atomic.h>
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "bananakit_misc.h"
#include "banana_string.h"
#include "rc_station.h"
#include "rc_vehicle_common.h"

void rc_station_interrupt(void);
static void update_keyboard_inputs(void);
static void update_communication(int64_t timestamp);
static void update_state_machine(void);
static void render(void);
static void generate_steer_effect(char *out, int sz);
static void generate_throttle_effect(char *out, int sz);

extern callstack_t Callstack;
extern bananakit_io_t IO;
static RF24 RF_radio(NRF24_CE_PIN, NRF24_CSN_PIN);
rc_station_t Station;
static int64_t __Timestamp, currTimestamp;
static int64_t prevUpdateTimestamp, prevRenderTimestamp;
static int32_t sequenceID;
static int16_t frameCounter;

void rc_station_init(void) {
    // Serial.begin(UART_SPEED_BPS);

    // NRF24 Initialize:
    RF_radio.begin();
    RF_radio.openWritingPipe((const uint8_t *) NRF24_ADDR_BASE);
    RF_radio.openReadingPipe(1, (const uint8_t *) NRF24_ADDR_VEHICLE);
    RF_radio.setPALevel(RF24_PA_HIGH);
    RF_radio.startListening();

    // Init global variables:
    __Timestamp             = 0;
    currTimestamp           = 0;
    prevUpdateTimestamp     = 0;
    prevRenderTimestamp     = 0;
    sequenceID              = 0;
    frameCounter            = 0;

    // Init rc station variables:
    memset(&Station.status, 0, sizeof(rc_station_status_t)); // clear all status bits
    Station.status.sync_with_vehicle = 1;
    Station.twist_x                     = 0.0;
    Station.twist_yaw                   = 0.0;
    Station.tx_rpy                      = 0.0;
    Station.tyaw_rpy                    = 0.0;
    Station.joy_neutral_pos_x           = 635;
    Station.joy_neutral_pos_y           = 645;
    Station.debug_code                  = 0;
    Station.sm_state                    = SM_UNCONNECT;

    // Init joystick:
    pinMode(JOY_SW, INPUT_PULLUP);
    pinMode(JOY_PUSH_BUTTON_0, INPUT_PULLUP);
    pinMode(JOY_PUSH_BUTTON_1, INPUT_PULLUP);

    // Attach interrupt handler:
    IO.interrupt_callback = rc_station_interrupt;

    IO.lcd_clear_callback();
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "rc init done"
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();
}

node_status_t rc_station_update(void) {
    node_status_t next_state = NODE_RUNNING;

    currTimestamp = __Timestamp;
    if(prevUpdateTimestamp == 0) {
        // First iteration, skip:
        prevUpdateTimestamp = currTimestamp;
        prevRenderTimestamp = currTimestamp;
        return next_state;
    }

    if(currTimestamp - prevUpdateTimestamp >= MAIN_LOOP_PERIOD) {
        update_keyboard_inputs();
        update_communication(currTimestamp);
        update_state_machine();
        prevUpdateTimestamp = currTimestamp;
    }

    if(currTimestamp - prevRenderTimestamp >= RENDER_LOOP_PERIOD) {
        render();
        prevRenderTimestamp = currTimestamp;
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

void rc_station_resume(void) {
}

void rc_station_exit(void) {
    // TODO: turn off nrf24
    IO.interrupt_callback = NULL;
}

void rc_station_interrupt(void) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        __Timestamp++;
    }
}


///////////////////////////////////////////////////////////////////
// Local functions:
static void update_keyboard_inputs(void) {
    float analog_value_f;
    int16_t volt0_adc;
    float max_linear_velocity;
    float axis_range_f;
    int16_t adc_value;

    // Compute rotation twist command:
    // analogRead() => [0, 1023]
    // neutral_pos = 635
    // 
    analog_value_f = (float) (analogRead(JOY_VRY) - Station.joy_neutral_pos_y);
    if(analog_value_f >= 0) {
        axis_range_f = 1024.0 - (float) Station.joy_neutral_pos_y;
    } else {
        axis_range_f = (float) Station.joy_neutral_pos_y;
    }
    Station.steer_percent = analog_value_f / axis_range_f;
    Station.twist_yaw = analog_value_f / axis_range_f * MAX_ANGULAR_VEL;

    // Compute speed limit values:
    // Station.speed_multi = ((float) analogRead(VOLT0_READ_PIN)) / 1024.0;
    volt0_adc = analogRead(VOLT0_READ_PIN);
    if(volt0_adc >= 0 && volt0_adc < 200) {
        Station.gear = 0;
        max_linear_velocity = GEAR0_SPEED;
    } else if(volt0_adc >= 200 && volt0_adc < 400) {
        Station.gear = 1;
        max_linear_velocity = GEAR1_SPEED;
    } else if(volt0_adc >= 400 && volt0_adc < 600) {
        Station.gear = 2;
        max_linear_velocity = GEAR2_SPEED;
    } else if(volt0_adc >= 600 && volt0_adc < 800) {
        Station.gear = 3;
        max_linear_velocity = GEAR3_SPEED;
    } else if(volt0_adc >= 800) {
        Station.gear = 4;
        max_linear_velocity = GEAR4_SPEED;
    }
    
    // Compute longitudinal twist command:
    // Longitudinal corresponds to joystick x-axis, positive value forward
    analog_value_f = (float) (analogRead(JOY_VRX) - Station.joy_neutral_pos_x);
    if(analog_value_f >= 0) {
        axis_range_f = 1024.0 - (float) Station.joy_neutral_pos_x;
    } else {
        axis_range_f = (float) Station.joy_neutral_pos_x;
    }
    Station.throttle_percent = analog_value_f / axis_range_f;
    Station.twist_x = analog_value_f / axis_range_f * max_linear_velocity;

    // Detect push button press event:
    Station.status.func_key1_pressed = 0;
    Station.status.func_key2_pressed = 0;
    Station.status.func_key3_pressed = 0;
    if(digitalRead(JOY_PUSH_BUTTON_0) == LOW) {
        delay(20);
        if(digitalRead(JOY_PUSH_BUTTON_0) == LOW) {
            Station.status.func_key1_pressed = 1;
        }
    }
    if(digitalRead(JOY_PUSH_BUTTON_1) == LOW) {
        delay(20);
        if(digitalRead(JOY_PUSH_BUTTON_1) == LOW) {
            Station.status.func_key2_pressed = 1;
        }
    }
    if(digitalRead(JOY_SW) == LOW) {
        delay(20);
        if(digitalRead(JOY_SW) == LOW) {
            Station.status.func_key3_pressed = 1;
        }
    }
}

static void update_communication(int64_t timestamp) {
    v2s_frame_t v2sframe;
    s2v_frame_t s2vframe;

    if(RF_radio.available()) {

        // Receive v2b frame from vehicle Arduino:
        RF_radio.read(&v2sframe, sizeof(v2s_frame_t));

        if(abs(timestamp - v2sframe.timestamp) < 1000 && v2sframe.header == FRAME_HEADER_ID) {
            Station.status.navigate_running                 = v2sframe.status.navigate_running;
            Station.status.gps_initialized                  = v2sframe.status.gps_initialized;
            Station.status.gps_data_valid                   = v2sframe.status.gps_data_valid;
            Station.vehicle_coordinate.lat_north_positive   = v2sframe.status.lat_north_positive;
            Station.vehicle_coordinate.lon_east_positive    = v2sframe.status.lon_east_positive;
            Station.vehicle_coordinate.lat_degree           = v2sframe.lat_degree;
            Station.vehicle_coordinate.lon_degree           = v2sframe.lon_degree;
            Station.vehicle_coordinate.lat_minute           = ((double) v2sframe.lat_minute_int / GPS_F2I_MULTI);
            Station.vehicle_coordinate.lon_minute           = ((double) v2sframe.lon_minute_int / GPS_F2I_MULTI);

            Station.ekf_x       = ((float) v2sframe.ekf_x_int)      / XY_F2I_MULTI;
            Station.ekf_y       = ((float) v2sframe.ekf_y_int)      / XY_F2I_MULTI;
            Station.ekf_yaw     = ((float) v2sframe.ekf_yaw_int)    / YAW_F2I_MULTI;
            Station.ekf_vyaw    = ((float) v2sframe.ekf_vyaw_int)   / ANG_VEL_F2I_MULTI;
            Station.ekf_v       = ((float) v2sframe.ekf_v_int)      / LIN_VEL_F2I_MULTI;

            Station.tx_rpy              = ((float) v2sframe.twist_x_int)    / LIN_VEL_F2I_MULTI;
            Station.tyaw_rpy            = ((float) v2sframe.twist_yaw_int)  / YAW_F2I_MULTI;
            Station.dist2goal           = ((float) v2sframe.dist2goal_int)  / XY_F2I_MULTI;
            Station.waypoint_index      = v2sframe.waypoint_index;
            Station.waypoint_list_sz    = v2sframe.waypoint_list_sz;
            Station.left_pwm            = v2sframe.left_pwm;
            Station.right_pwm           = v2sframe.right_pwm;
            Station.debug_code          = v2sframe.debug_code;
            Station.battery_adc_value   = v2sframe.battery_adc_value;
            Station.delta_ms            = v2sframe.delta_ms;
            Station.status.sync_with_vehicle = 1;
        } else {
            // If incoming data is out-of-date or frame header mismatch:
            // Station.status.sync_with_vehicle = 0;
        }
    } else {
        // Station.status.sync_with_vehicle = 0;
        // delay(5);
    }

    // Reply s2v frame to vehicle:
    s2vframe.header = FRAME_HEADER_ID;
    s2vframe.timestamp = timestamp;
    s2vframe.sequence_id = frameCounter;
    frameCounter++;

    s2vframe.status.cmd_navigate_start  = Station.status.cmd_navigate_start;
    s2vframe.status.cmd_navigate_cancel = Station.status.cmd_navigate_cancel;
    s2vframe.status.cmd_estop           = Station.status.cmd_estop;
    s2vframe.status.cmd_auto_mode       = Station.status.cmd_auto_mode;
    s2vframe.status.cmd_set_origin      = Station.status.cmd_set_origin;
    if(!Station.status.cmd_auto_mode) {
        s2vframe.twist_x_int            = (int16_t) (Station.twist_x * LIN_VEL_F2I_MULTI);
        s2vframe.twist_yaw_int          = (int16_t) (Station.twist_yaw * ANG_VEL_F2I_MULTI);
    }
    s2vframe.gear = Station.gear;

    RF_radio.stopListening();
    RF_radio.write(&s2vframe, sizeof(s2v_frame_t));
    RF_radio.startListening();

    // Clear these status after send:
    Station.status.cmd_navigate_start   = 0;
    Station.status.cmd_navigate_cancel  = 0;
    Station.status.cmd_auto_mode        = 0;
    Station.status.cmd_set_origin       = 0;
}

static void update_state_machine(void) {
    switch(Station.sm_state) {
        case SM_UNCONNECT:
            if(Station.status.sync_with_vehicle) {
                Station.sm_state = SM_MANUAL;
            }
            break;

        case SM_MANUAL:
            if(Station.status.func_key2_pressed) {
                // Switch to MANU_ESTOP:
                Station.status.cmd_estop = 1;
                Station.sm_state = SM_MANU_ESTOP_INIT;
            } else if(IO.keypress == PIC_PLAY) {
                // Request to start navigation:
                Station.status.cmd_navigate_start = 1;
            } else if(IO.keypress == PIC_CHAOS) {
                // Enter debug mode:
                Station.sm_state = SM_DEBUG1;
            }
            if(Station.status.navigate_running) {
                // Navigation is running:
                Station.status.cmd_navigate_start = 0;
                Station.sm_state = SM_NAVIGATE1;
            }
            break;

        case SM_MANU_ESTOP_INIT:
            if(!Station.status.func_key2_pressed) {
                // Wait for the estop button release:
                Station.sm_state = SM_MANU_ESTOP;
            }
            break;

        case SM_MANU_ESTOP:
            if(Station.status.func_key2_pressed) {
                // estop pressed the second time, cancel estop:
                Station.status.cmd_estop = 0;
                Station.sm_state = SM_MANUAL;
            }
            break;

        case SM_NAVIGATE1:
            if(Station.status.func_key2_pressed) {
                // E-stop:
                Station.sm_state = SM_NAV_ESTOP_INIT;
            } else if(Station.status.func_key1_pressed) {
                // Auto/manual mode switch:
                if(Station.status.cmd_auto_mode) {
                    Station.status.cmd_auto_mode = 0;
                } else {
                    Station.status.cmd_auto_mode = 1;
                }
            } else if(IO.keypress == PIC_PLAY) {
                // Request to stop navigation:
                Station.status.cmd_navigate_cancel = 1;
            } else if(Station.status.func_key1_pressed) {
                // Change to next screen:
                Station.sm_state = SM_NAVIGATE2;
            } else if(IO.keypress == PIC_CHAOS) {
                // Enter debug mode:
                Station.sm_state = SM_DEBUG1;
            }
            if(!Station.status.navigate_running) {
                Station.status.cmd_navigate_cancel = 0;
                Station.sm_state = SM_MANUAL;
            }
            break;

        case SM_NAVIGATE2:
            if(Station.status.func_key2_pressed) {
                // E-stop:
                Station.sm_state = SM_NAV_ESTOP;
            } else if(Station.status.func_key1_pressed) {
                // Auto/manual mode switch:
                if(Station.status.cmd_auto_mode) {
                    Station.status.cmd_auto_mode = 0;
                } else {
                    Station.status.cmd_auto_mode = 1;
                }
            } else if(IO.keypress == PIC_PLAY) {
                // Request to stop navigation:
                Station.status.cmd_navigate_cancel = 1;
            } else if(Station.status.func_key1_pressed) {
                // Change to previous screen:
                Station.sm_state = SM_NAVIGATE1;
            }
            if(!Station.status.navigate_running) {
                Station.status.cmd_navigate_cancel = 0;
                Station.sm_state = SM_MANUAL;
            }
            break;

        case SM_NAV_ESTOP_INIT:
            if(!Station.status.func_key2_pressed) {
                Station.sm_state = SM_NAV_ESTOP;
            }
            break;

        case SM_NAV_ESTOP:
            if(Station.status.func_key2_pressed) {
                Station.sm_state = SM_NAVIGATE1;
            }
            break;

        case SM_DEBUG1:
            if(IO.keypress == PIC_PLUS) {
                Station.sm_state = SM_DEBUG2;
            } else if(IO.keypress == PIC_MINUS) {
                Station.sm_state = SM_DEBUG3;
            } else if(IO.keypress == PIC_CHAOS) {
                if(Station.status.navigate_running) {
                    Station.sm_state = SM_NAVIGATE1;
                } else {
                    Station.sm_state = SM_MANUAL;
                }
            }
            break;

        case SM_DEBUG2:
            if(IO.keypress == PIC_PLUS) {
                Station.sm_state = SM_DEBUG3;
            } else if(IO.keypress == PIC_MINUS) {
                Station.sm_state = SM_DEBUG1;
            } else if(IO.keypress == PIC_CHAOS) {
                if(Station.status.navigate_running) {
                    Station.sm_state = SM_NAVIGATE1;
                } else {
                    Station.sm_state = SM_MANUAL;
                }
            }
            break;

        case SM_DEBUG3:
            if(IO.keypress == PIC_PLUS) {
                Station.sm_state = SM_DEBUG1;
            } else if(IO.keypress == PIC_MINUS) {
                Station.sm_state = SM_DEBUG2;
            } else if(IO.keypress == PIC_CHAOS) {
                if(Station.status.navigate_running) {
                    Station.sm_state = SM_NAVIGATE1;
                } else {
                    Station.sm_state = SM_MANUAL;
                }
            }
            break;
    }
}

static void render(void) {
    char floatbuf[LCD_BUF_SIZE];
    int offset;
    IO.lcd_clear_callback();

    // Line 0:
    switch(Station.sm_state) {
        case SM_UNCONNECT:
        case SM_MANUAL:
        case SM_MANU_ESTOP:
        case SM_NAVIGATE1:
        case SM_NAVIGATE2:
        case SM_NAV_ESTOP:
        case SM_DEBUG1:
            float2str(GET_ADC_VOLT(Station.battery_adc_value), floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "BATT:%s     GPS:%s",
                Station.status.sync_with_vehicle ? floatbuf : "NA",
                Station.status.gps_data_valid ? "OK" : "OFF"
            );
            IO.flags = LCD_REFRESH_LINE0;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG2:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "GPS INIT:%d  VALID:%d",
                Station.status.gps_initialized,
                Station.status.gps_data_valid
            );
            IO.flags = LCD_REFRESH_LINE0;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG3:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "EKF"
            );
            IO.flags = LCD_REFRESH_LINE0;
            IO.lcd_refresh_callback();
            break;
        default:
            break; 
    }

    // Line 1:
    switch(Station.sm_state) {
        case SM_UNCONNECT:
        case SM_MANUAL:
        case SM_MANU_ESTOP:
        case SM_NAVIGATE1:
        case SM_NAVIGATE2:
        case SM_NAV_ESTOP:
        case SM_DEBUG1:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "MODE:%s    NAV:%s",
                Station.status.cmd_auto_mode ? "AUTO" : "MANU",
                Station.status.navigate_running ? "ON " : "OFF"
            );
            IO.flags = LCD_REFRESH_LINE1;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG2:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "ORIGIN:%d",
                Station.status.gps_origin_set
            );
            IO.flags = LCD_REFRESH_LINE1;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG3:
            offset = 0;
            float2str(Station.ekf_x, floatbuf, LCD_BUF_SIZE, 2);
            offset += snprintf(
                IO.lcd_buf+offset,
                LCD_BUF_SIZE-offset,
                "X:%s  ",
                floatbuf
            );
            float2str(Station.ekf_y, floatbuf, LCD_BUF_SIZE, 2);
            offset += snprintf(
                IO.lcd_buf+offset,
                LCD_BUF_SIZE-offset,
                "Y:%s",
                floatbuf
            );
            IO.flags = LCD_REFRESH_LINE1;
            IO.lcd_refresh_callback();
            break;
        default:
            break;
    }

    // Line 2:
    switch(Station.sm_state) {
        case SM_UNCONNECT:
        case SM_MANUAL:
        case SM_MANU_ESTOP:
        case SM_NAVIGATE1:
        case SM_NAVIGATE2:
        case SM_NAV_ESTOP:
            generate_throttle_effect(IO.lcd_buf, LCD_BUF_SIZE);
            IO.flags = LCD_REFRESH_LINE2;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG1:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "PWM      L:%d R:%d",
                Station.left_pwm,
                Station.right_pwm
            );
            IO.flags = LCD_REFRESH_LINE2;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG2:
            float2str(Station.vehicle_coordinate.lat_minute, floatbuf, LCD_BUF_SIZE, 5);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "LAT:%dD%sM  %c",
                Station.vehicle_coordinate.lat_degree,
                floatbuf,
                Station.vehicle_coordinate.lat_north_positive ? 'N' : 'S'
            );
            IO.flags = LCD_REFRESH_LINE2;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG3:
            offset = 0;
            float2str(Station.ekf_yaw, floatbuf, LCD_BUF_SIZE, 1);
            offset += snprintf(
                IO.lcd_buf+offset,
                LCD_BUF_SIZE-offset,
                "Z:%s     ",
                floatbuf
            );
            float2str(Station.ekf_vyaw, floatbuf, LCD_BUF_SIZE, 1);
            offset += snprintf(
                IO.lcd_buf+offset,
                LCD_BUF_SIZE-offset,
                "VZ:%s",
                floatbuf
            );
            IO.flags = LCD_REFRESH_LINE2;
            IO.lcd_refresh_callback();
            break;
        default:
            break;
    }

    // Line 3
    switch(Station.sm_state) {
        case SM_UNCONNECT:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "  *NOT CONNECTED*"
            );
            IO.flags = LCD_REFRESH_LINE3;
            IO.lcd_refresh_callback();
            break;
        case SM_MANU_ESTOP:
        case SM_NAV_ESTOP:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "   *** E-STOP ***"
            );
            IO.flags = LCD_REFRESH_LINE3;
            IO.lcd_refresh_callback();
            break;
        case SM_MANUAL:
        case SM_NAVIGATE1:
            generate_steer_effect(IO.lcd_buf, LCD_BUF_SIZE);
            IO.flags = LCD_REFRESH_LINE3;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG1:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "DT:%d      DEBUG:%d",
                Station.delta_ms,
                Station.debug_code
            );
            IO.flags = LCD_REFRESH_LINE3;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG2:
            float2str(Station.vehicle_coordinate.lon_minute, floatbuf, LCD_BUF_SIZE, 5);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "LON:%dD%sM  %c",
                Station.vehicle_coordinate.lon_degree,
                floatbuf,
                Station.vehicle_coordinate.lon_east_positive ? 'E' : 'W'
            );
            IO.flags = LCD_REFRESH_LINE3;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG3:
            float2str(Station.ekf_v, floatbuf, LCD_BUF_SIZE, 2);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "V:%s",
                floatbuf
            );
            IO.flags = LCD_REFRESH_LINE3;
            IO.lcd_refresh_callback();
            break;
        default:
            break;
    }

    // On power-on, not connected to vehicle (SM_UNCONNECT):
    // ====================
    // BATT:NA      GPS:NA
    // MODE:MANUAL  NAV:OFF
    // ACCEL+++++   GEAR:10
    //   *NOT CONNECTED*
    // ====================

    // On power-on, receive signal, press accelerator (reverse) and steer slightly right (SM_MANUAL):
    // ====================
    // BATT:11.6    GPS:OK
    // MODE:MANUAL  NAV:OFF
    // REVER+++     GEAR:5
    //        STEER >>>
    // ====================

    // Manual estop (SM_MANU_ESTOP):
    // ====================
    // BATT:11.6    GPS:OK
    // MODE:MANUAL  NAV:OFF
    // REVER        GEAR:5
    //    *** E-STOP ***
    // ====================

    // Enter navigation mode, first page (SM_NAVIGATE1):
    // ====================
    // BATT:11.6    GPS:OK
    // MODE:AUTO    NAV:ON
    // ACCEL+++     GEAR:5
    //        STEER >>>
    // ====================

    // Enter navigation mode, second page (SM_NAVIGATE2):
    // ====================
    // BATT:11.6    GPS:OK
    // MODE:AUTO    NAV:ON
    // WPT:1/5     VEL:0.6
    // DIST:20.1
    // ====================

    // Press E-stop (SM_NAV_ESTOP):
    // ====================
    // BATT:11.6    GPS:OK
    // MODE:MANUAL  NAV:OFF
    // 
    //    *** E-STOP ***
    // ====================

    // Debug mode1 (SM_DEBUG1):
    // ====================
    // BATT:11.6    GPS:OK
    // MODE:MANUAL  NAV:OFF
    // PWM      L:255 R:105
    // DT:20        DEBUG:2
    // ====================

    // Debug mode2 (SM_DEBUG2):
    // ====================
    // GPS INIT:1  VALID:1
    // ORIGIN:1
    // LAT:51D35.12345M  N
    // LON:113D30.56123M W
    // ====================

    // Debug mode3 (SM_DEBUG3):
    // ====================
    // EKF
    // X:20.12      Y:10.34
    // Z:0.21     VZ:Z10.12
    // V:0.2
    // ====================
}

static void generate_steer_effect(char *out, int sz) {
    // Generate the following effect in the output buffer:
    //  <<<<< ROTATE >>>>>    
    // ^    ^ ^      ^    ^
    // 0    5 7      14   19
    char arrows[6];
    int i;
    int num_arrows = (int) roundf(fabs(Station.steer_percent) * 6.0);
    num_arrows = max(0, min(5, num_arrows));

    if(Station.steer_percent >= 0) {
        // Right rotation
        for(i = 0; i < num_arrows; i++) {
            arrows[i] = '>';
        }
        arrows[i] = '\0';
        snprintf(out, sz, "       ROTATE %s ", arrows);
    } else {
        // Left rotation
        // <<<<<0
        // ^    ^
        // 0    5
        for(i = 0; i < 5; i++) {
            if(i >= 5 - num_arrows) {
                arrows[i] = '<';
            } else {
                arrows[i] = ' ';
            }
        }
        for(i = 5 - num_arrows; i < 5; i++) {
            arrows[i] = '<';
        }
        arrows[i] = '\0';
        snprintf(out, sz, " %s ROTATE", arrows);
    }
}

static void generate_throttle_effect(char *out, int sz) {
    // Generate the following effect in the output buffer:
    // ACCEL+++++   GEAR:5
    // REVER+++++   GEAR:5
    // ^    ^   ^   ^     ^
    // 0    a   b   c     19

    int num_arrows;
    int i;
    char arrows[6];

    num_arrows = (int) roundf(fabs(Station.throttle_percent) * 6.0);
    num_arrows = max(0, min(5, num_arrows));

    // Clear output:
    memset(arrows, ' ', 6);

    for(i = 0; i < num_arrows; i++) {
        arrows[i] = '+';
    }
    arrows[5] = '\0';

    if(Station.throttle_percent >= 0) {
        // Forward acceleration
        snprintf(out, sz, "ACCEL%s    GEAR:%d", arrows, Station.gear);
    } else {
        // Reverse
        snprintf(out, sz, "REVER%s    GEAR:%d", arrows, Station.gear);
    }
}

#endif