#ifndef __RC_STATION__
#define __RC_STATION__

#include <stdint.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <CRC.h>

#define UART_SPEED_BPS 115200
#define NRF24_ADDR_BASE "00001"
#define NRF24_ADDR_VEHICLE "00002"
#define FRAME_HEADER_ID 0xA0
#define GPS_F2I_MULTI ((float) 1000000) // convert float to int for communication
#define MAX_LINEAR_SPEED (0.5) // m/s
#define MAX_ANGULAR_SPEED (3.14) // rad/s
#define ADC_R1 30000.0 // for compute battery voltage
#define ADC_R2 7500.0  // for compute battery voltage

#define RC_SUCCESS      1
#define RC_ERR_NUL_PTR  0

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

// RC station variables:
typedef struct __rc_station__ {
    int64_t timestamp;
    int16_t frame_counter;
    status_code_t status_code;
    int16_t twist_x, twist_y, twist_yaw;
    float goal_latitude, goal_longitude, goal_orientation;
    int32_t goal_latitude_int, goal_longitude_int, goal_orientation_int;
    int16_t adc_value;
    int32_t latitude_int, longitude_int, altitude_int, orientation_int;
    uint8_t sm_state;
    uint8_t toggle_ts_sync;
    int joy_neutral_pos_x;
    int joy_neutral_pos_y;
    float max_linear_speed;  // m/sec
    float max_angular_speed; // rad/sec
} rc_station_t;

void rc_station_init(void);
node_status_t rc_station_update(void);
void rc_station_resume(void);
void rc_station_exit(void);

#endif