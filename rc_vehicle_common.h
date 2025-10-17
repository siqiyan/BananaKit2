// This header file defines shared parameters and data structures for rc vehicle and station.
// Version: 1.0.0

#ifndef __RC_VEHICLE_COMMON__
#define __RC_VEHICLE_COMMON__

#include <stdint.h>

#define NRF24_ADDR_BASE "00001"
#define NRF24_ADDR_VEHICLE "00002"
#define FRAME_HEADER_ID 0xA0
#define GPS_F2I_MULTI ((float) 1000000) // convert GPS float to int for communication
#define CMD_VEL_MULTI ((float) 100)     // convert cmd_vel float to int
#define MAX_LINEAR_SPEED        0.5     // m/s
#define MAX_ANGULAR_SPEED       3.14    // rad/s
#define ADC_R1                  30000.0 // for compute battery voltage
#define ADC_R2                  7500.0  // for compute battery voltage
#define ADC_REF_VOLT            5.0

typedef struct __status_code__ {
    uint8_t estop:              1;
    uint8_t navigate:           1;
    uint8_t navigate_reply:     1;
    uint8_t sync_with_laptop:   1;
    uint8_t sync_with_vehicle:  1;
} status_code_t;

// (wireless) Vehicle-to-Station dataframe:
// (nRF24 has built-in checksum verification)
typedef struct __attribute__((__packed__)) __v2s_frame {
    uint8_t header;
    status_code_t status_code;
    int64_t timestamp;
    int32_t sequence_id;
    int32_t latitude_int;
    int32_t longitude_int;
    int32_t altitude_int;
    int32_t orientation_int;
    int16_t adc_value;
    uint8_t sm_state;
} v2s_frame_t;

// (wireless) Station-to-Vehicle dataframe:
// (nRF24 has built-in checksum verification)
typedef struct __attribute__((__packed__)) __s2v_frame {
    uint8_t header;
    status_code_t status_code;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t twist_x;
    int16_t twist_y;
    int16_t twist_yaw;
    int32_t goal_latitude_int;
    int32_t goal_longitude_int;
    int32_t goal_orientation_int;
} s2v_frame_t;

// (UART) Station-to-laptop dataframe:
typedef struct __attribute__((__packed__)) __s2l_frame {
    uint8_t header;
    status_code_t status_code;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t adc_value;
    int32_t latitude_int;
    int32_t longitude_int;
    int32_t orientation_int;
    uint8_t sm_state;
    uint8_t checksum;
} s2l_frame_t;

// (UART) Laptop-to-Station dataframe:
typedef struct __attribute__((__packed__)) __l2s_frame {
    uint8_t header;
    status_code_t status_code;
    int64_t timestamp;
    int32_t sequence_id;
    int32_t goal_latitude_int;
    int32_t goal_longitude_int;
    int32_t goal_orientation_int;
    uint8_t checksum;
} l2s_frame_t;


void init_status_code(status_code_t *code);
void init_v2s_frame(v2s_frame_t *frame);
void init_s2v_frame(s2v_frame_t *frame);
void init_s2l_frame(s2l_frame_t *frame);
void init_l2s_frame(l2s_frame_t *frame);
float compute_batt_volt(int16_t adc_value);

#endif