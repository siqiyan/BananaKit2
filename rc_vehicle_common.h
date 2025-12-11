// This header file defines shared parameters and data structures for rc vehicle and station.

#ifndef __RC_VEHICLE_COMMON__
#define __RC_VEHICLE_COMMON__

#include <stdint.h>

#define NRF24_ADDR_BASE         "00001"
#define NRF24_ADDR_VEHICLE      "00002"
#define FRAME_HEADER_ID         0xA0
#define GPS_F2I_MULTI           1000000.0   // convert GPS float to int for communication
#define RAD_F2I_MULTI           1000.0      // convert radian from float to int
#define CMD_VEL_F2I_MULTI       100.0       // convert cmd_vel float to int
#define MAX_LINEAR_SPEED        0.5         // m/s
#define MAX_ANGULAR_SPEED       3.14        // rad/s

// For compute battery voltage:
// https://lastminuteengineers.com/voltage-sensor-arduino-tutorial/
#define REF_VOLTAGE             5.0
#define ADC_R1                  47000.0
#define ADC_R2                  10000.0
#define GET_ADC_VOLT(x)         (x * REF_VOLTAGE / 1024.0 * (ADC_R1 + ADC_R2) / ADC_R2)

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

    int32_t delta_ms;
    int32_t dist2goal_int;
    int16_t adc_value;
    uint8_t debug_code;
} v2s_frame_t;

// (wireless) Station-to-Vehicle dataframe:
// (nRF24 has built-in checksum verification)
typedef struct {
    uint8_t estop:                  1;
    uint8_t auto_mode:              1;
    uint8_t cmd_navigate_start:     1;
    uint8_t cmd_navigate_cancel:    1;
} s2v_status_t;

typedef struct __attribute__((__packed__)) __s2v_frame {
    s2v_status_t status;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t twist_x_int;
    int16_t twist_yaw_int;
    int16_t speed_multi_int;
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