// This header file defines shared parameters and data structures for rc vehicle and station.

#ifndef __RC_VEHICLE_COMMON__
#define __RC_VEHICLE_COMMON__

// Hardware addresses:
#define NRF24_ADDR_BASE         "00001"
#define NRF24_ADDR_VEHICLE      "00002"

// Vehicle physics parameters:
#define WHEEL_DIAMETER              0.115
#define WHEEL_TRACK                 0.5
#define ENCODER_RESOLUTION          7392
#define MAX_NAVIGATION_RANGE        200.0   // meter
#define SPEED_REDUCTION_RANGE       3.0     // meter, full speed if goal dist exceed this range
#define GOAL_REACH_RANGE            0.5     // meter, tolerance value to determine if goal is reached range
#define MAX_LINEAR_VEL              0.5     // meter/sec for auto mode
#define MIN_LINEAR_VEL              0.1     // meter/sec, reduction speed, for auto mode
#define MAX_ANGULAR_VEL             3.14    // rad/s, auto mode and manual mode
#define RAMP_LINEAR_ACCEL           0.1     // meter/sec^2
#define MAX_WAYPOINT_SZ             10

// Manual speed at different gear level:
#define GEAR0_SPEED                 0.1
#define GEAR1_SPEED                 0.3
#define GEAR2_SPEED                 0.5
#define GEAR3_SPEED                 0.9
#define GEAR4_SPEED                 1.5

// Convert between float and integer for data communication
// After conversion to integer the number after float point will be discard during transmiting
// To keep more precision increase the constant, but on Arduino Nano the total float precision is 6-7 digits (if I remember)
// So the maximum is 10,000,000.0
// To convert float => int do multiplication
// To convert int => float do division
#define GPS_F2I_MULTI           1000000.0   // GPS latitude and longitude
#define YAW_F2I_MULTI           1000000.0   // Yaw angle in radian
#define XY_F2I_MULTI            100000.0      // X-Y coordinate in meter
#define LIN_VEL_F2I_MULTI       1000.0       // linear velocity in meter/sec
#define ANG_VEL_F2I_MULTI       1000.0       // angular velocity in rad/sec

// For compute battery voltage:
// https://lastminuteengineers.com/voltage-sensor-arduino-tutorial/
#define REF_VOLTAGE             5.0
#define ADC_R1                  47000.0
#define ADC_R2                  10000.0
#define GET_ADC_VOLT(x)         (x * REF_VOLTAGE / 1024.0 * (ADC_R1 + ADC_R2) / ADC_R2)

#include <stdint.h>
#define FRAME_HEADER_ID         0xA0
// (wireless) Vehicle-to-Station dataframe:
// (nRF24 has built-in checksum verification)
typedef struct {
    uint8_t navigate_running:       1;
    uint8_t gps_data_valid:         1;
    uint8_t gps_initialized:        1;
    uint8_t lat_north_positive:     1;
    uint8_t lon_east_positive:      1;
} v2s_status_t;

typedef struct __attribute__((__packed__)) __v2s_frame {
    uint8_t header;         // header identifier, set to FRAME_HEADER_ID
    v2s_status_t status;
    int64_t timestamp;
    int32_t sequence_id;

    int16_t lat_degree;
    int16_t lon_degree;
    int32_t lat_minute_int;
    int32_t lon_minute_int;
    int32_t ekf_x_int;
    int32_t ekf_y_int;
    int32_t ekf_yaw_int;
    int32_t ekf_v_int;
    int32_t ekf_vyaw_int;
    uint8_t left_pwm;
    uint8_t right_pwm;
    int16_t twist_x_int;
    int16_t twist_yaw_int;

    int8_t waypoint_index;
    int8_t waypoint_list_sz;
    int32_t delta_ms;
    int32_t dist2goal_int;
    int16_t battery_adc_value;
    uint8_t debug_code;
} v2s_frame_t;

// (wireless) Station-to-Vehicle dataframe:
// (nRF24 has built-in checksum verification)
typedef struct {
    uint8_t cmd_estop:              1;
    uint8_t cmd_auto_mode:          1;
    uint8_t cmd_navigate_start:     1;
    uint8_t cmd_navigate_cancel:    1;
    uint8_t cmd_sync_timestamp:     1;
    uint8_t cmd_set_origin:         1;
} s2v_status_t;

typedef struct __attribute__((__packed__)) __s2v_frame {
    uint8_t header;         // header identifier, set to FRAME_HEADER_ID
    s2v_status_t status;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t twist_x_int;
    int16_t twist_yaw_int;
    uint8_t gear;
} s2v_frame_t;

// (UART) Station-to-laptop dataframe:
// typedef struct __attribute__((__packed__)) __s2l_frame {
//     uint8_t header;
//     packet_status_t status;
//     int64_t timestamp;
//     int32_t sequence_id;
//     int16_t adc_value;
//     int32_t latitude_int;
//     int32_t longitude_int;
//     int32_t orientation_int;
//     uint8_t sm_state;
//     uint8_t checksum;
// } s2l_frame_t;

// (UART) Laptop-to-Station dataframe:
// typedef struct __attribute__((__packed__)) __l2s_frame {
//     uint8_t header;
//     packet_status_t status;
//     int64_t timestamp;
//     int32_t sequence_id;
//     int32_t goal_latitude_int;
//     int32_t goal_longitude_int;
//     int32_t goal_orientation_int;
//     uint8_t checksum;
// } l2s_frame_t;

#endif