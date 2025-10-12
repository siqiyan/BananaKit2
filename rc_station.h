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

#define RC_SUCCESS      1
#define RC_ERR_NUL_PTR  0

// (Vehicle | Serial) Arduino-to-on-board computer dataframe:
typedef struct __attribute__((__packed__)) __a2o_frame {
    uint8_t header;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t adc_value;
    int16_t twist_x;
    int16_t twist_y;
    int16_t twist_yaw;
    int32_t goal_latitude_int;
    int32_t goal_longitude_int;
    int32_t goal_orientation_int;
    uint8_t cmdkey;
    uint8_t checksum;
} a2o_frame_t;

// (Vehicle | Serial) On-board computer-to-Arduino dataframe:
typedef struct __attribute__((__packed__)) __o2a_frame {
    uint8_t header;
    int64_t timestamp;
    int32_t sequence_id;
    int32_t latitude_int;
    int32_t longitude_int;
    int32_t orientation_int;
    uint8_t sm_state;
    uint8_t checksum;
} o2a_frame_t;

// (Vehicle/Base | nRF24L01) Vehicle-to-base dataframe:
typedef struct __attribute__((__packed__)) __v2b_frame {
    uint8_t header;
    int64_t timestamp;
    int32_t sequence_id;
    int32_t latitude_int;
    int32_t longitude_int;
    int32_t orientation_int;
    int16_t adc_value;
    uint8_t sm_state;
} v2b_frame_t;

// (Vehicle/Base | nRF24L01) Base-to-vehicle dataframe:
typedef struct __attribute__((__packed__)) __b2v_frame {
    uint8_t header;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t twist_x;
    int16_t twist_y;
    int16_t twist_yaw;
    int32_t goal_latitude_int;
    int32_t goal_longitude_int;
    int32_t goal_orientation_int;
    uint8_t cmdkey;
} b2v_frame_t;

// (Base | Serial) Arduino-to-laptop dataframe:
typedef struct __attribute__((__packed__)) __a2l_frame {
    uint8_t header;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t adc_value;
    int32_t latitude_int;
    int32_t longitude_int;
    int32_t orientation_int;
    uint8_t sm_state;
    uint8_t checksum;
} a2l_frame_t;

// (Base | Serial) Laptop-to-Arduino dataframe:
typedef struct __attribute__((__packed__)) __l2a_frame {
    uint8_t header;
    int64_t timestamp;
    int32_t sequence_id;
    int16_t twist_x;
    int16_t twist_y;
    int16_t twist_yaw;
    int32_t goal_latitude_int;
    int32_t goal_longitude_int;
    int32_t goal_orientation_int;
    uint8_t cmdkey;
    uint8_t checksum;
} l2a_frame_t;

// RC station variables:
typedef struct __rc_station__ {
    int64_t timestamp;
    int32_t sequence_id;
    int16_t frame_counter;
    uint8_t cmd_toggle;
    int16_t twist_x, twist_y, twist_yaw;
    float goal_latitude, goal_longitude, goal_orientation;
    int32_t goal_latitude_int, goal_longitude_int, goal_orientation_int;
    int16_t adc_value;
    float latitude, longitude, altitude, orientation;
    int32_t latitude_int, longitude_int, orientation_int;
    uint8_t sm_state;
    uint8_t toggle_ts_sync;
    int8_t sync_with_laptop;
    int8_t sync_with_vehicle;
    int joy_neutral_pos_x;
    int joy_neutral_pos_y;
    float speed_multi;
} rc_station_t;

void rc_station_init(void);
node_status_t rc_station_update(void);
void rc_station_resume(void);
void rc_station_exit(void);

#endif