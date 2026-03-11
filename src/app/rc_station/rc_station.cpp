#include "bananakit.h"
#ifdef ENABLE_RC_STATION

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <Arduino.h>
#include <util/atomic.h>
#include "lib/bananakit/callstack.h"
#include "lib/bananakit/menu.h"
#include "lib/bananakit/bananakit_io.h"
#include "lib/bananakit/bananakit_misc.h"
#include "lib/bananakit/banana_string.h"
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
static void process_navi_frame(const navigation_status_t *frame);

extern callstack_t Callstack;
extern bananakit_io_t IO;
static RF24 RF_radio(NRF24_CE_PIN, NRF24_CSN_PIN);
static rc_station_t Station;
static command_frame_t AckPacket;
static int32_t currTimestamp;
static int32_t prevTimestamp;
static int16_t sequenceID;
static uint8_t iterCounter;
static int16_t frameCounter;
static uint8_t newPacketEvent;
static int32_t LostPacketCounter;

static void incoming_packet_handle() {
    newPacketEvent = 1;
}

void rc_station_init(void) {
    Serial.begin(9600);

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
    currTimestamp           = 0;
    prevTimestamp           = 0;
    sequenceID              = 0;
    LostPacketCounter       = 0;
    frameCounter            = 0;
    iterCounter             = 0;

    // Init rc station variables:
    memset(&Station, 0, sizeof(rc_station_t));
    Station.status.is_connected         = 0;
    Station.joy_neutral_pos_x           = 498;
    Station.joy_neutral_pos_y           = 506;
    Station.debug_code                  = 0;
    // Station.sm_state                    = SM_UNCONNECT;
    Station.sm_state = SM_MANUAL1;

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

    currTimestamp = millis();
    if(prevTimestamp == 0) {
        // First iteration, skip:
        prevTimestamp = currTimestamp;
        return next_state;
    }

    update_communication_irq();

    if(currTimestamp - prevTimestamp >= MAIN_LOOP_PERIOD) {

        // Use a scheduling table to balance CPU load
        // loop rate 10Hz, each iter%10 run at 1Hz, can control task range 1-10Hz with step size 1Hz
        switch(iterCounter) {
            case 0:
                update_keyboard_inputs();
                update_state_machine();
                break;
            case 1:
                render();
                break;
            case 2:
                update_keyboard_inputs();
                update_state_machine();
                break;
            case 3:
                break;
            case 4:
                update_keyboard_inputs();
                update_state_machine();
                break;
            case 5:
                break;
            case 6:
                update_keyboard_inputs();
                update_state_machine();
                break;
            case 7:
                render();
                break;
            case 8:
                update_keyboard_inputs();
                update_state_machine();
                break;
            case 9:
                break;
            default:
                break;
        }

        prevTimestamp = currTimestamp;
        iterCounter = (iterCounter + 1) % 10;
    }

    // update_state_machine();
    // update_communication_irq();
    // update_keyboard_inputs();

    // if(currTimestamp - prevRenderTimestamp >= 500) {
    //     render();

    //     prevRenderTimestamp = currTimestamp;
    // }

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
}

///////////////////////////////////////////////////////////////////
// Local functions:
#define DEADZONE_JOY 10
static void update_keyboard_inputs(void) {
    int16_t raw_offset;

    // Joystick Y-Axis:
    //     Left most = 0
    //     Right most = 1023
    // Right hand coordinate:
    //     Left rotate > 0
    //     Right rotate < 0
    // 
    raw_offset = Station.joy_neutral_pos_y - analogRead(JOY_VRY);
    if(abs(raw_offset) < DEADZONE_JOY) {
        Station.cmd_yaw_int = 0;
    } else {
        if (raw_offset >= 0) {
            // Turn left (right hand, positive yaw):
            // Input Range: [DEADZONE_JOY, joy_neutral_pos_y]
            // Output: [0, 127]
            Station.cmd_yaw_int = (int8_t)map(raw_offset, DEADZONE_JOY, Station.joy_neutral_pos_y, 0, 127);
        } else {
            // Turn right (right hand, negative yaw):
            // Input Range: [joy_neutral_pos_y - 1023, -DEADZONE_JOY-1]
            // Output: [-128, -1]
            Station.cmd_yaw_int = (int8_t)map(raw_offset, Station.joy_neutral_pos_y - 1023, -DEADZONE_JOY - 1, -128, -1);
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

    // Detect push button press event:
    Station.status.button_left_pressed = 0;
    Station.status.button_right_pressed = 0;
    Station.status.button_joy_pressed = 0;
    if(digitalRead(JOY_PUSH_BUTTON_0) == LOW) {
        delay(20);
        if(digitalRead(JOY_PUSH_BUTTON_0) == LOW) {
            Station.status.button_left_pressed = 1;
        }
    }
    if(digitalRead(JOY_PUSH_BUTTON_1) == LOW) {
        delay(20);
        if(digitalRead(JOY_PUSH_BUTTON_1) == LOW) {
            Station.status.button_right_pressed = 1;
        }
    }
    if(digitalRead(JOY_SW) == LOW) {
        delay(20);
        if(digitalRead(JOY_SW) == LOW) {
            Station.status.button_joy_pressed = 1;
        }
    }
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

    Station.cmd_x_reply             = frame->cmd_x_reply;
    Station.cmd_yaw_reply           = frame->cmd_yaw_reply;
    Station.left_pwm                = frame->left_pwm;
    Station.right_pwm               = frame->right_pwm;
    if(frame->status.left_dir) {
        Station.left_pwm_signed     = ((int16_t) frame->left_pwm);
    } else {
        Station.left_pwm_signed     = -((int16_t) frame->left_pwm);
    }
    if(frame->status.right_dir) {
        Station.right_pwm_signed    = ((int16_t) frame->right_pwm);
    } else {
        Station.right_pwm_signed    = -((int16_t) frame->right_pwm);
    }
    Station.debug_code              = frame->debug_code;
    Station.battery_adc_value       = frame->battery_adc_value;
    Station.status.is_connected     = 1;
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
    Station.status.compass_valid                    = frame->status.compass_valid;
    Station.status.gps_origin_set                   = frame->status.origin_initialized;
    Station.vehicle_coordinate.lat_north_positive   = frame->status.lat_north_positive;
    Station.vehicle_coordinate.lon_east_positive    = frame->status.lon_east_positive;
    Station.vehicle_coordinate.lat_degree           = frame->lat_degree;
    Station.vehicle_coordinate.lon_degree           = frame->lon_degree;
    Station.vehicle_coordinate.lat_minute           = frame->lat_minute;
    Station.vehicle_coordinate.lon_minute           = frame->lon_minute;
    Station.compass_yaw                             = frame->compass_yaw;
    Station.local_x                                 = frame->local_x;
    Station.local_y                                 = frame->local_y;
    Station.status.is_connected = 1;
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
    Station.status.auto_mode            = frame->status.auto_mode;
    Station.dist2goal                   = frame->dist2goal;
    Station.yaw_err                     = frame->yaw_err;
    Station.waypoint_index              = frame->waypoint_index;
    Station.waypoint_list_sz            = frame->waypoint_list_sz;
    Station.status.is_connected = 1;
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
    frame->sequence_id = frameCounter++;
    frame->status.cmd_navigate_start  = Station.status.cmd_navigate_start;
    frame->status.cmd_navigate_cancel = Station.status.cmd_navigate_cancel;
    frame->status.cmd_auto_mode       = Station.status.cmd_auto_mode;
    frame->status.cmd_manu_mode       = Station.status.cmd_manu_mode;
    frame->status.cmd_set_origin      = Station.status.cmd_set_origin;
    frame->cmd_x_int                  = Station.cmd_x_int;
    frame->cmd_yaw_int                = Station.cmd_yaw_int;
    frame->gear                       = Station.gear;
    frame->checksum = compute_checksum((char *) frame, sizeof(command_frame_t));
}

static void update_state_machine(void) {
    // Interaction logic:
    // Manual mode (press left) -> manual info mode
    // Manual mode (press right) -> navi mode
    //
    // Navi mode (press left) -> navi info mode
    // Navi mode (press right) -> manual mode
    // Navi mode (move joystick) -> pause
    // Navi mode (press joy sw) -> restore
    //
    // Manual info mode (press left) -> loop
    // Manual info mode (press right) -> navi mode
    //
    // Navi info mode (press left) -> loop
    // Navi info mode (press right) -> manual mode
    // Navi info mode (press joystick) -> pause
    // Navi info mode (press joy sw) -> restore

    switch(Station.sm_state) {
        case SM_UNCONNECT:
            if(Station.status.is_connected) {
                Station.sm_state = SM_MANUAL1;
            }
            break;

        case SM_MANUAL1:
        case SM_MANUAL2:
        case SM_MANUAL3:
        case SM_MANUAL4:
            if(Station.status.button_left_pressed && Station.status.button_right_pressed) {
                // Request to start navigation:
                Station.status.cmd_navigate_start = 1;
            } else if(Station.status.button_right_pressed) {
                // Switch between auto/manual mode:
                if(Station.status.auto_mode) {
                    Station.status.cmd_auto_mode = 0;
                    Station.status.cmd_manu_mode = 1;
                } else {
                    Station.status.cmd_manu_mode = 0;
                    Station.status.cmd_auto_mode = 1;
                }
            } else if(Station.status.button_left_pressed) {
                // Loop info screen:
                if(Station.sm_state == SM_MANUAL1) {
                    Station.sm_state = SM_MANUAL2;
                } else if(Station.sm_state == SM_MANUAL2) {
                    Station.sm_state = SM_MANUAL3;
                } else if(Station.sm_state == SM_MANUAL3) {
                    Station.sm_state = SM_MANUAL4;
                } else if(Station.sm_state == SM_MANUAL4) {
                    Station.sm_state = SM_MANUAL1;
                }
            }
            if(Station.status.button_joy_pressed) {
                // Request to set GPS origin:
                Station.status.cmd_set_origin = 1;
            }
            if(Station.status.navigate_running) {
                // Navigation is running, change to navigation state:
                Station.status.cmd_navigate_start = 0;
                Station.sm_state = SM_NAVIGATE1;
            }
            if(Station.status.gps_origin_set) {
                Station.status.cmd_set_origin = 0;
            }
            break;

        case SM_NAVIGATE1:
        case SM_NAVIGATE2:
        case SM_NAVIGATE3:
            if(Station.status.button_left_pressed && Station.status.button_right_pressed) {
                // Request to stop navigation:
                Station.status.cmd_navigate_cancel = 1;
            } else if(Station.status.button_right_pressed) {
                // Switch between auto/manual mode:
                if(Station.status.auto_mode) {
                    Station.status.cmd_auto_mode = 0;
                    Station.status.cmd_manu_mode = 1;
                } else {
                    Station.status.cmd_manu_mode = 0;
                    Station.status.cmd_auto_mode = 1;
                }
            } else if(Station.status.button_left_pressed) {
                // Loop info screen:
                if(Station.sm_state == SM_NAVIGATE1) {
                    Station.sm_state = SM_NAVIGATE2;
                } else if(Station.sm_state == SM_NAVIGATE2) {
                    Station.sm_state = SM_NAVIGATE3;
                } else if(Station.sm_state == SM_NAVIGATE3) {
                    Station.sm_state = SM_NAVIGATE1;
                }
            }
            if(Station.cmd_x_int != 0 || Station.cmd_yaw_int != 0) {
                // Joystick moved, request to switch manual mode (but don't stop navigation):
                Station.status.cmd_auto_mode = 0;
            }
            if(!Station.status.navigate_running) {
                // Navigation stopped, change to manual state:
                Station.status.cmd_navigate_cancel = 0;
                Station.sm_state = SM_MANUAL1;
            }
            break;
    }
}

#define ALIGN_BUF_SIZE 21
static void render(void) {
    char floatbuf[LCD_BUF_SIZE];
    char align_buf[ALIGN_BUF_SIZE];
    IO.lcd_clear_callback();

    // Line 0:
    switch(Station.sm_state) {
        case SM_UNCONNECT:
        case SM_MANUAL1:
        case SM_NAVIGATE1:
            float2str(GET_ADC_VOLT(Station.battery_adc_value), floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "BATT:%s",
                Station.status.is_connected ? floatbuf : "NA"
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "GPS:%s",
                Station.status.gps_initialized ? (
                    Station.status.gps_origin_set ? "ORI" : "OK"
                ) : "NA"
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL2:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "CMD X:%d",
                Station.cmd_x_int
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "YAW:%d",
                Station.cmd_yaw_int
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL3:
        case SM_MANUAL4:
        case SM_NAVIGATE3:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "GPS:%s",
                Station.status.gps_initialized ? (
                    Station.status.gps_origin_set ? "ORI" : "OK"
                ) : "NA"
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "VALID:%d",
                Station.status.gps_data_valid
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_NAVIGATE2:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "WPT:%d/%d",
                Station.waypoint_index+1,
                Station.waypoint_list_sz
            );
            float2str(Station.dist2goal, floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "GOAL:%s",
                floatbuf
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        default:
            IO.lcd_buf[0] = '\0';
            break; 
    }
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    // Line 1:
    switch(Station.sm_state) {
        case SM_UNCONNECT:
        case SM_MANUAL1:
        case SM_NAVIGATE1:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "MODE:%s",
                Station.status.auto_mode ? "AUTO" : "MANUAL"
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "NAV:%s",
                Station.status.navigate_running ? "ON " : "OFF"
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL2:
            float2str(Station.cmd_x_reply, floatbuf, LCD_BUF_SIZE, 3);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "RPY X:%s",
                floatbuf
            );
            float2str(Station.cmd_yaw_reply, floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "YAW:%s",
                floatbuf
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL3:
        case SM_MANUAL4:
        case SM_NAVIGATE2:
        case SM_NAVIGATE3:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "CPS:%s",
                Station.status.compass_valid ? "OK" : "NA"
            );
            float2str(Station.compass_yaw, floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "YAW:%s",
                floatbuf
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        default:
            IO.lcd_buf[0] = '\0';
            break;
    }
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    // Line 2:
    switch(Station.sm_state) {
        case SM_UNCONNECT:
        case SM_MANUAL1:
            generate_throttle_effect(IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL2:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "L:%d",
                Station.left_pwm_signed
            );
            snprintf(
                align_buf,
                LCD_BUF_SIZE,
                "SEQ:%d",
                sequenceID
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL3:
        case SM_NAVIGATE3:
            float2str(Station.vehicle_coordinate.lat_minute, floatbuf, LCD_BUF_SIZE, 5);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "LAT:%dD%sM",
                Station.vehicle_coordinate.lat_degree,
                floatbuf
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "%c",
                Station.vehicle_coordinate.lat_north_positive ? 'N' : 'S'
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL4:
        case SM_NAVIGATE2:
            float2str(Station.local_x, floatbuf, LCD_BUF_SIZE, 3);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "X:%s",
                floatbuf
            );
            float2str(Station.local_y, floatbuf, LCD_BUF_SIZE, 3);
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "Y:%s",
                floatbuf
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_NAVIGATE1:
            float2str(Station.dist2goal, floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "GOAL:%s",
                floatbuf
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "GEAR:%d",
                Station.gear
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        default:
            IO.lcd_buf[0] = '\0';
            break;
    }
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();

    // Line 3
    switch(Station.sm_state) {
        case SM_UNCONNECT:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "  *NOT CONNECTED*"
            );
            break;
        case SM_MANUAL1:
            generate_steer_effect(IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL2:
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "R:%d",
                Station.right_pwm_signed
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "DEBUG:%d",
                Station.debug_code
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_MANUAL3:
        case SM_NAVIGATE3:
            float2str(Station.vehicle_coordinate.lon_minute, floatbuf, LCD_BUF_SIZE, 5);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "LON:%dD%sM",
                Station.vehicle_coordinate.lon_degree,
                floatbuf
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "%c",
                Station.vehicle_coordinate.lon_east_positive ? 'E' : 'W'
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        //case SM_MANUAL4:
            //float2str(Station.std, floatbuf, LCD_BUF_SIZE, 1);
            //snprintf(
                //IO.lcd_buf,
                //LCD_BUF_SIZE,
                //"STD:%s",
                //floatbuf
            //);
            //break;
        case SM_NAVIGATE2:
            float2str(Station.yaw_err, floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "ERR:%s",
                floatbuf
            );
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "DEBUG:%d",
                Station.debug_code
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        case SM_NAVIGATE1:
            float2str(Station.cmd_x_reply, floatbuf, LCD_BUF_SIZE, 3);
            snprintf(
                IO.lcd_buf,
                LCD_BUF_SIZE,
                "RPY X:%s",
                floatbuf
            );
            float2str(Station.cmd_yaw_reply, floatbuf, LCD_BUF_SIZE, 1);
            snprintf(
                align_buf,
                ALIGN_BUF_SIZE,
                "YAW:%s",
                floatbuf
            );
            right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
            break;
        default:
            IO.lcd_buf[0] = '\0';
            break;
    }
    IO.flags = LCD_REFRESH_LINE3;
    IO.lcd_refresh_callback();

    // SM_UNCONNECT:
    // ====================
    // BATT:NA       GPS:NA
    // MODE:MANUAL  NAV:OFF
    // ACCEL+++++  GEAR:100
    //   *NOT CONNECTED*
    // ====================

    // SM_MANUAL1:
    // ====================
    // BATT:11.6    GPS:ORI
    // MODE:MANUAL  NAV:OFF
    // REVER+++    GEAR:255
    //        STEER >>>
    // ====================

    // SM_MANUAL2:
    // ====================
    // CMD X:0.5   YAW:-0.2
    // RPY X:0.4   YAW:-0.6
    // L:-255     SEQ:12345
    // R:100      DEBUG:128
    // ====================

    // SM_MANUAL3:
    // ====================
    // GPS:ON       VALID:1
    // CPS:OK     YAW: 1.32
    // LAT:51D35.12345M   N
    // LON:113D30.56123M  W
    // ====================

    // SM_MANUAL4:
    // ====================
    // GPS:ORI      VALID:1
    // CPS:OK     YAW: 1.32
    // X:123.45  Y:-1234.5
    //
    // ====================

    // SM_NAVIGATE1:
    // ====================
    // BATT:11.6    GPS:ORI
    // MODE:AUTO     NAV:ON
    // GOAL:12.3   GEAR:255
    // RPY X:0.4   YAW:-0.6
    // ====================

    // SM_NAVIGATE2:
    // ====================
    // WPT:1/5    GOAL:20.1
    // CPS:OK     YAW: 1.32
    // X:123.45   Y:-1234.5
    // ERR:3.14   DEBUG:255
    // ====================

    // SM_NAVIGATE3:
    // ====================
    // GPS:ORI      VALID:1
    // CPS:OK     YAW: 1.32
    // LAT:51D35.12345M   N
    // LON:113D30.56123M  W
    // ====================
}

static void generate_steer_effect(char *out, int sz) {
    // Generate the following effect in the output buffer:
    //  <<<<< ROTATE >>>>>    
    // ^    ^ ^      ^    ^
    // 0    5 7      14   19
    char arrows[6];
    int i;
    int num_arrows;

    // cmd_yaw_int:
    //     left  [0, 127]
    //     right [-1, -128]

    if(Station.cmd_yaw_int < 0) {
        // Right rotation
        num_arrows = map(abs(Station.cmd_yaw_int) - 1, 0, 128, 0, 6);
        for(i = 0; i < num_arrows; i++) {
            arrows[i] = '>';
        }
        arrows[i] = '\0';
        snprintf(out, sz, "       ROTATE %s ", arrows);
    } else {
        // Left rotation
        num_arrows = map(Station.cmd_yaw_int, 0, 128, 0, 6);
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
    char right_align[LCD_BUF_SIZE];

    num_arrows = map(abs(Station.cmd_x_int), 0, 128, 0, 6);

    // Clear output:
    memset(arrows, ' ', 6);

    for(i = 0; i < num_arrows; i++) {
        arrows[i] = '+';
    }
    arrows[5] = '\0';

    if(Station.cmd_x_int >= 0) {
        // Forward acceleration
        snprintf(out, sz, "ACCEL%s", arrows);
        snprintf(right_align, LCD_BUF_SIZE, "GEAR:%d", Station.gear);
        right_align_overlay(right_align, out, sz);
    } else {
        // Reverse
        snprintf(out, sz, "REVER%s", arrows);
        snprintf(right_align, LCD_BUF_SIZE, "GEAR:%d", Station.gear);
        right_align_overlay(right_align, out, sz);
    }
}

#endif
