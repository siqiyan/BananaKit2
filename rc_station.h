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
    SM_MANUAL,
    SM_MANU_ESTOP_INIT,
    SM_MANU_ESTOP,
    SM_NAVIGATE1,
    SM_NAVIGATE2,
    SM_NAV_ESTOP_INIT,
    SM_NAV_ESTOP,
    SM_SETTING,
    SM_DEBUG1,
    SM_DEBUG2,
    SM_DEBUG3
} station_state_machine_t;

typedef struct {
    uint8_t navigate_running:       1;
    uint8_t gps_data_valid:         1;
    uint8_t gps_initialized:        1;
    uint8_t gps_origin_set:         1;
    uint8_t s2v_received:           1;
    uint8_t cmd_auto_mode:          1;
    uint8_t cmd_navigate_start:     1;
    uint8_t cmd_navigate_cancel:    1;
    uint8_t cmd_estop:              1;
    // uint8_t cmd_sync_timestamp:     1;
    uint8_t cmd_set_origin:         1;
    uint8_t sync_with_vehicle:      1;
    uint8_t func_key1_pressed:      1;
    uint8_t func_key2_pressed:      1;
    uint8_t func_key3_pressed:      1;
    uint8_t recv_header_err:        1;
} rc_station_status_t;

// RC station variables:
typedef struct __rc_station__ {
    rc_station_status_t status;
    float               twist_x, twist_yaw;
    float               twist_x_reply, twist_yaw_reply;
    int16_t             battery_adc_value;
    geo_coordinate_t    vehicle_coordinate;
    int16_t             joy_neutral_pos_x;
    int16_t             joy_neutral_pos_y;
    uint8_t             gear;
    // float               throttle_percent;
    // float               steer_percent;
    // int8_t              throttle_percent_int;
    // int8_t              steer_percent_int;
    float               ekf_x;
    float               ekf_y;
    float               ekf_yaw;
    float               ekf_v;
    float               ekf_vyaw;
    uint8_t             left_pwm;
    uint8_t             right_pwm;
    int32_t             delta_ms;
    float               dist2goal;
    int8_t              waypoint_index;
    int8_t              waypoint_list_sz;
    uint8_t             debug_code;
    int32_t             recv_frame_counter;
    int16_t             packet_timeout_counter;
    station_state_machine_t  sm_state;
} rc_station_t;

void rc_station_init(void);
node_status_t rc_station_update(void);
void rc_station_resume(void);
void rc_station_exit(void);

#endif