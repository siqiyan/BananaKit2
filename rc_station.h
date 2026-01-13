#ifndef __RC_STATION__
#define __RC_STATION__

#include <stdint.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <CRC.h>

#include "rc_vehicle_common.h"
#include "gnss_reader.h"

// #define UART_SPEED_BPS 115200
#define MAIN_LOOP_PERIOD        100 // 10Hz, 100ms period
#define MAIN_LOOP_MAX           150 // upper bound, trigger warning
// #define RENDER_LOOP_PERIOD      500 // 2Hz

typedef enum {
    SM_UNCONNECT,
    SM_MANUAL1,
    SM_MANUAL2,
    SM_MANUAL3,
    SM_MANUAL4,
    SM_NAVIGATE1,
    SM_NAVIGATE2,
    SM_NAVIGATE3
} station_state_machine_t;

// typedef struct {
//     uint8_t navigate_running:       1;
//     uint8_t gps_data_valid:         1;
//     uint8_t gps_initialized:        1;
//     uint8_t gps_origin_set:         1;
//     uint8_t is_connected:           1;
//     uint8_t cmd_auto_mode:          1;
//     uint8_t auto_mode:              1;
//     uint8_t cmd_navigate_start:     1;
//     uint8_t cmd_navigate_cancel:    1;
//     uint8_t cmd_set_origin:         1;
//     uint8_t sync_with_vehicle:      1;
//     uint8_t button_left_pressed:    1;
//     uint8_t button_right_pressed:   1;
//     uint8_t button_joy_pressed:     1;
//     uint8_t recv_header_err:        1;
// } rc_station_status_t;

// RC station variables:
typedef struct __rc_station__ {
    struct {
        uint8_t navigate_running:       1;
        uint8_t gps_data_valid:         1;
        uint8_t gps_initialized:        1;
        uint8_t gps_origin_set:         1;
        uint8_t compass_valid:          1;
        uint8_t is_connected:           1;
        uint8_t cmd_auto_mode:          1;
        uint8_t auto_mode:              1;
        uint8_t cmd_navigate_start:     1;
        uint8_t cmd_navigate_cancel:    1;
        uint8_t cmd_set_origin:         1;
        uint8_t sync_with_vehicle:      1;
        uint8_t button_left_pressed:    1;
        uint8_t button_right_pressed:   1;
        uint8_t button_joy_pressed:     1;
        uint8_t recv_header_err:        1;
    } status;

    int8_t              cmd_x_int;
    int8_t              cmd_yaw_int;
    float               cmd_x_reply;
    float               cmd_yaw_reply;
    int16_t             battery_adc_value;
    geo_coordinate_t    vehicle_coordinate;
    float               compass_yaw;
    float               local_x;
    float               local_y;
    int16_t             joy_neutral_pos_x;
    int16_t             joy_neutral_pos_y;
    uint8_t             gear;
    uint8_t             left_pwm;
    uint8_t             right_pwm;
    int16_t             left_pwm_signed;
    int16_t             right_pwm_signed;
    float               dist2goal;
    int8_t              waypoint_index;
    int8_t              waypoint_list_sz;
    uint8_t             debug_code;
    station_state_machine_t  sm_state;
} rc_station_t;

void rc_station_init(void);
node_status_t rc_station_update(void);
void rc_station_resume(void);
void rc_station_exit(void);

#endif