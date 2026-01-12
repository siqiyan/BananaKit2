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

static void rc_station_interrupt(void);
static void update_keyboard_inputs(void);
static void update_communication(void);
static void update_communication_irq(void);
static void update_state_machine(void);
static void render(void);
static void generate_steer_effect(char *out, int sz);
static void generate_throttle_effect(char *out, int sz);
static void generate_cmd_packet(command_frame_t *frame);
static void process_status_packet(const vehicle_status_t *frame);
static void process_gps_frame(const gps_status_t *frame);
static void process_ekf_frame(const ekf_status_t *frame);
static void process_navi_frame(const navigation_status_t *frame);

extern callstack_t Callstack;
extern bananakit_io_t IO;
static RF24 RF_radio(NRF24_CE_PIN, NRF24_CSN_PIN);
static rc_station_t Station;
static command_frame_t AckPacket;
static int32_t __Timestamp, currTimestamp;
static int32_t prevUpdateTimestamp, prevRenderTimestamp;
static int16_t sequenceID;
static int16_t frameCounter;
static uint8_t iterCounter;
static uint8_t warnIter;
static uint8_t newPacketEvent;
static int32_t LostPacketCounter;

static void incoming_packet_handle() {
    newPacketEvent = 1;
}

void rc_station_init(void) {
    // Serial.begin(UART_SPEED_BPS);

    // NRF24 Initialize:
    RF_radio.begin();
    RF_radio.setPALevel(RF24_PA_HIGH);
    RF_radio.setDataRate(RF24_250KBPS);
    RF_radio.enableDynamicPayloads();
    RF_radio.enableAckPayload();
    RF_radio.maskIRQ(true, true, false);
    RF_radio.stopListening((const uint8_t *) NRF24_ADDR_BASE);
    RF_radio.openReadingPipe(1, (const uint8_t *) NRF24_ADDR_VEHICLE);
 
    // Create initial AckPacket:
    generate_cmd_packet(&AckPacket);
    RF_radio.writeAckPayload(1, &AckPacket, sizeof(command_frame_t));
    RF_radio.startListening();

    // NRF24 interrupt:
    newPacketEvent = 0;
    pinMode(NRF24_IRQ_PIN, INPUT_PULLUP);
    attachInterrupt(
        digitalPinToInterrupt(NRF24_IRQ_PIN),
        incoming_packet_handle,
        FALLING
    );

    // Init global variables:
    __Timestamp             = 0;
    currTimestamp           = 0;
    prevUpdateTimestamp     = 0;
    prevRenderTimestamp     = 0;
    sequenceID              = 0;
    LostPacketCounter       = 0;
    frameCounter            = 0;
    iterCounter             = 0;
    warnIter                = 9;

    // Init rc station variables:
    memset(&Station.status, 0, sizeof(rc_station_status_t)); // clear all status bits
    Station.status.sync_with_vehicle    = 1;
    // Station.twist_x                     = 0.0;
    // Station.twist_yaw                   = 0.0;
    // Station.tx_rpy                      = 0.0;
    // Station.tyaw_rpy                    = 0.0;
    Station.joy_neutral_pos_x           = 635;
    Station.joy_neutral_pos_y           = 645;
    Station.debug_code                  = 0;
    Station.sm_state                    = SM_UNCONNECT;

    // Init joystick:
    // pinMode(JOY_SW, INPUT_PULLUP); // disabled, this pin conflict with nrf24 IRQ pin
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

    // currTimestamp = __Timestamp;
    currTimestamp = millis();
    if(prevUpdateTimestamp == 0) {
        // First iteration, skip:
        prevUpdateTimestamp = currTimestamp;
        prevRenderTimestamp = currTimestamp;
        return next_state;
    }

    update_state_machine();
    update_communication_irq();
    update_keyboard_inputs();

    if(currTimestamp - prevRenderTimestamp >= 500) {
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
    // ATOMIC_BLOCK(ATOMIC_FORCEON) {
    //     __Timestamp++;
    // }
}


///////////////////////////////////////////////////////////////////
// Local functions:
#define DEADZONE_JOY 10
static void update_keyboard_inputs(void) {
    // float analog_value_f;
    // int16_t volt0_adc;
    // float max_linear_velocity;
    // float axis_range_f;
    int16_t raw_offset;

    raw_offset = analogRead(JOY_VRY) - Station.joy_neutral_pos_y;
    if (abs(raw_offset) < DEADZONE_JOY) {
        Station.cmd_yaw_int = 0;
    } else {
        if (raw_offset > 0) {
            Station.cmd_yaw_int = (int8_t)map(raw_offset, DEADZONE_JOY, 1023 - Station.joy_neutral_pos_y, 0, 127);
        } else {
            Station.cmd_yaw_int = (int8_t)map(raw_offset, -Station.joy_neutral_pos_y, -DEADZONE_JOY, -128, 0);
        }
    }

    raw_offset = analogRead(JOY_VRX) - Station.joy_neutral_pos_x;
    if (abs(raw_offset) < DEADZONE_JOY) {
        Station.cmd_x_int = 0;
    } else {
        if (raw_offset > 0) {
            Station.cmd_x_int = (int8_t)map(raw_offset, DEADZONE_JOY, 1023 - Station.joy_neutral_pos_x, 0, 127);
        } else {
            Station.cmd_x_int = (int8_t)map(raw_offset, -Station.joy_neutral_pos_x, -DEADZONE_JOY, -128, 0);
        }
    }

    raw_offset = analogRead(VOLT0_READ_PIN);
    if (abs(raw_offset) < DEADZONE_JOY) {
        Station.gear = 0;
    } else {
        Station.gear = (uint8_t)map(raw_offset, DEADZONE_JOY, 1023, 0, 255);
    }

    // Compute rotation twist command:
    // analog_value_f = (float) (analogRead(JOY_VRY) - Station.joy_neutral_pos_y);
    // if(analog_value_f >= 0) {
    //     axis_range_f = 1024.0 - (float) Station.joy_neutral_pos_y;
    // } else {
    //     axis_range_f = (float) Station.joy_neutral_pos_y;
    // }
    // if(!Station.status.sync_with_vehicle) {
    //     // Not receiving feedback command from vehicle, use local joystick command directly:
    //     Station.steer_percent = analog_value_f / axis_range_f;
    // }
    // Station.twist_yaw = analog_value_f / axis_range_f * MAX_ANGULAR_VEL;
    // Station.cmd_yaw = (int8_t) (analog_value_f / axis_range_f * 128.0);

    // Compute speed limit values:
    // raw_offset = analogRead(VOLT0_READ_PIN);
    // if(raw_offset >= 0 && raw_offset < 400) {
    //     Station.gear = 1;
    //     // max_linear_velocity = GEAR0_SPEED;
    // } else if(raw_offset >= 400 && raw_offset < 800) {
    //     Station.gear = 2;
    //     // max_linear_velocity = GEAR1_SPEED;
    // // } else if(raw_offset >= 400 && raw_offset < 600) {
    //     // Station.gear = 3;
    //     // max_linear_velocity = GEAR2_SPEED;
    // // } else if(raw_offset >= 600 && raw_offset < 800) {
    //     // Station.gear = 4;
    //     // max_linear_velocity = GEAR3_SPEED;
    // } else if(raw_offset >= 800) {
    //     Station.gear = 3;
    //     // max_linear_velocity = GEAR4_SPEED;
    // }
    
    // Compute longitudinal twist command:
    // Longitudinal corresponds to joystick x-axis, positive value forward
    // analog_value_f = (float) (analogRead(JOY_VRX) - Station.joy_neutral_pos_x);
    // if(analog_value_f >= 0) {
    //     axis_range_f = 1024.0 - (float) Station.joy_neutral_pos_x;
    // } else {
    //     axis_range_f = (float) Station.joy_neutral_pos_x;
    // }
    // if(!Station.status.sync_with_vehicle) {
    //     // Not receiving feedback command from vehicle, use local joystick command directly:
    //     Station.throttle_percent = analog_value_f / axis_range_f;
    // }
    // Station.twist_x = analog_value_f / axis_range_f * max_linear_velocity;
    // Station.cmd_x = (int8_t) (analog_value_f / axis_range_f * 128.0);

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
    // if(digitalRead(JOY_SW) == LOW) {
    //     delay(20);
    //     if(digitalRead(JOY_SW) == LOW) {
    //         Station.status.func_key3_pressed = 1;
    //     }
    // }
}

static void process_status_packet(const vehicle_status_t *frame) {
    if(compute_checksum((char *) frame, sizeof(vehicle_status_t)) != frame->checksum) {
        return;
    }
    if(frame->sequence_id != sequenceID) {
        LostPacketCounter++;
        sequenceID = frame->sequence_id;
    }
    sequenceID++;

    Station.cmd_x_reply       = frame->cmd_x_reply;
    Station.cmd_yaw_reply     = frame->cmd_yaw_reply;
    Station.left_pwm            = frame->left_pwm;
    Station.right_pwm           = frame->right_pwm;
    Station.debug_code          = frame->debug_code;
    Station.battery_adc_value   = frame->battery_adc_value;
}

static void process_gps_frame(const gps_status_t *frame) {
    if(compute_checksum((char *) frame, sizeof(gps_status_t)) != frame->checksum) {
        return;
    }
    if(frame->sequence_id != sequenceID) {
        LostPacketCounter++;
        sequenceID = frame->sequence_id;
    }
    sequenceID++;

    Station.status.gps_initialized                  = frame->status.gps_initialized;
    Station.status.gps_data_valid                   = frame->status.gps_data_valid;
    Station.vehicle_coordinate.lat_north_positive   = frame->status.lat_north_positive;
    Station.vehicle_coordinate.lon_east_positive    = frame->status.lon_east_positive;
    Station.vehicle_coordinate.lat_degree           = frame->lat_degree;
    Station.vehicle_coordinate.lon_degree           = frame->lon_degree;
    Station.vehicle_coordinate.lat_minute           = frame->lat_minute;
    Station.vehicle_coordinate.lon_minute           = frame->lon_minute;
}

static void process_ekf_frame(const ekf_status_t *frame) {
    if(compute_checksum((char *) frame, sizeof(ekf_status_t)) != frame->checksum) {
        return;
    }
    if(frame->sequence_id != sequenceID) {
        LostPacketCounter++;
        sequenceID = frame->sequence_id;
    }
    sequenceID++;
    // Station.ekf_x       = frame->ekf_x;
    // Station.ekf_y       = frame->ekf_y;
    // Station.ekf_yaw     = frame->ekf_yaw;
    // Station.ekf_vyaw    = frame->ekf_vyaw;
    // Station.ekf_v       = frame->ekf_v;
}

static void process_navi_frame(const navigation_status_t *frame) {
    if(compute_checksum((char *) frame, sizeof(navigation_status_t)) != frame->checksum) {
        return;
    }
    if(frame->sequence_id != sequenceID) {
        LostPacketCounter++;
        sequenceID = frame->sequence_id;
    }
    sequenceID++;
    Station.status.navigate_running     = frame->status.navigate_running;
    Station.dist2goal                   = frame->dist2goal;
    Station.waypoint_index              = frame->waypoint_index;
    Station.waypoint_list_sz            = frame->waypoint_list_sz;
}

static void update_communication_irq(void) {
    uint8_t recv_buf[32];
    uint8_t sz;

    if(newPacketEvent) {
        newPacketEvent = 0;
        RF_radio.clearStatusFlags();

        // Station do not send any data and wait for vehicle packet arrives first:
        if(RF_radio.available()) {
            sz = RF_radio.getDynamicPayloadSize();
            if(sz < 1) {
                // Corrupt packet:
                return;
            }

            RF_radio.read(recv_buf, sz);

            // Auto ACK already replied by hardware, put a new ACK packet for next auto reply:
            RF_radio.writeAckPayload(1, &AckPacket, sizeof(command_frame_t));

            switch(recv_buf[0]) {
                case FRAMEID_VEHICLE_STATUS:
                    process_status_packet((const vehicle_status_t *) recv_buf);
                    Station.status.recv_header_err = 0;
                    break;
                case FRAMEID_GPS_STATUS:
                    process_gps_frame((const gps_status_t *) recv_buf);
                    Station.status.recv_header_err = 0;
                    break;
                case FRAMEID_EKF_STATUS:
                    process_ekf_frame((const ekf_status_t *) recv_buf);
                    Station.status.recv_header_err = 0;
                    break;
                case FRAMEID_NAVIGATION_STATUS:
                    process_navi_frame((const navigation_status_t *) recv_buf);
                    Station.status.recv_header_err = 0;
                    break;
                default:
                    // Error: header unrecognized
                    Station.status.recv_header_err = 1;
            }
        }
    }

    // Generate ack packet for next communicate event:
    generate_cmd_packet(&AckPacket);
}

static void generate_cmd_packet(command_frame_t *frame) {
    // After established connection with vehicle, reply s2v packet to vehicle:
    frame->header = FRAMEID_COMMAND;
    // frame->timestamp = timestamp;
    // frame.sequence_id = frameCounter;
    frameCounter++;

    frame->status.cmd_navigate_start  = Station.status.cmd_navigate_start;
    frame->status.cmd_navigate_cancel = Station.status.cmd_navigate_cancel;
    frame->status.cmd_estop           = Station.status.cmd_estop;
    frame->status.cmd_auto_mode       = Station.status.cmd_auto_mode;
    frame->status.cmd_set_origin      = Station.status.cmd_set_origin;
    // if(!Station.status.cmd_auto_mode) {
        // frame.twist_x_int            = (int16_t) (Station.twist_x * LIN_VEL_F2I_MULTI);
        // frame.twist_yaw_int          = (int16_t) (Station.twist_yaw * ANG_VEL_F2I_MULTI);
    // }
    // frame->twist_x            = Station.twist_x;
    // frame->twist_yaw          = Station.twist_yaw;
    frame->cmd_x_int                = Station.cmd_x_int;
    frame->cmd_yaw_int              = Station.cmd_yaw_int;
    frame->gear                     = Station.gear;
    frame->checksum = compute_checksum((char *) frame, sizeof(command_frame_t));
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
                // Station.status.sync_with_vehicle ? Station.battery_adc_value : -1,
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
                "ORIGIN:%d  SEQ:%d",
                Station.status.gps_origin_set,
                Station.recv_frame_counter
            );
            IO.flags = LCD_REFRESH_LINE1;
            IO.lcd_refresh_callback();
            break;
        case SM_DEBUG3:
            // offset = 0;
            // float2str(Station.ekf_x, floatbuf, LCD_BUF_SIZE, 2);
            // offset += snprintf(
            //     IO.lcd_buf+offset,
            //     LCD_BUF_SIZE-offset,
            //     "X:%s  ",
            //     floatbuf
            // );
            // float2str(Station.ekf_y, floatbuf, LCD_BUF_SIZE, 2);
            // offset += snprintf(
            //     IO.lcd_buf+offset,
            //     LCD_BUF_SIZE-offset,
            //     "Y:%s",
            //     floatbuf
            // );
            // IO.flags = LCD_REFRESH_LINE1;
            // IO.lcd_refresh_callback();
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
                "L:%d R:%d  s2v:%d",
                Station.left_pwm,
                Station.right_pwm,
                Station.status.s2v_received
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
            // offset = 0;
            // float2str(Station.ekf_yaw, floatbuf, LCD_BUF_SIZE, 1);
            // offset += snprintf(
            //     IO.lcd_buf+offset,
            //     LCD_BUF_SIZE-offset,
            //     "Z:%s     ",
            //     floatbuf
            // );
            // float2str(Station.ekf_vyaw, floatbuf, LCD_BUF_SIZE, 1);
            // offset += snprintf(
            //     IO.lcd_buf+offset,
            //     LCD_BUF_SIZE-offset,
            //     "VZ:%s",
            //     floatbuf
            // );
            // IO.flags = LCD_REFRESH_LINE2;
            // IO.lcd_refresh_callback();
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
            // float2str(Station.ekf_v, floatbuf, LCD_BUF_SIZE, 2);
            // snprintf(
            //     IO.lcd_buf,
            //     LCD_BUF_SIZE,
            //     "V:%s",
            //     floatbuf
            // );
            // IO.flags = LCD_REFRESH_LINE3;
            // IO.lcd_refresh_callback();
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
    // L:-255 R:105  s2v:OK
    //              DEBUG:2
    // ====================

    // Debug mode2 (SM_DEBUG2):
    // ====================
    // GPS INIT:1  VALID:1
    // ORIGIN:1  SEQ:12345
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
    int num_arrows = (int) roundf(fabs(Station.cmd_yaw_reply) / MAX_ANGULAR_VEL * 6.0);
    num_arrows = max(0, min(5, num_arrows));

    if(Station.cmd_yaw_reply >= 0) {
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

    num_arrows = (int) roundf(fabs(Station.cmd_x_reply) / MAX_LINEAR_VEL * 6.0);
    num_arrows = max(0, min(5, num_arrows));

    // Clear output:
    memset(arrows, ' ', 6);

    for(i = 0; i < num_arrows; i++) {
        arrows[i] = '+';
    }
    arrows[5] = '\0';

    if(Station.cmd_x_reply >= 0) {
        // Forward acceleration
        snprintf(out, sz, "ACCEL%s    GEAR:%d", arrows, Station.gear);
    } else {
        // Reverse
        snprintf(out, sz, "REVER%s    GEAR:%d", arrows, Station.gear);
    }
}

#endif