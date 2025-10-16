#ifndef __RC_STATION__
#define __RC_STATION__

#include <stdint.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <CRC.h>

#include "rc_vehicle_common.h"

#define UART_SPEED_BPS 115200

#define RC_SUCCESS      1
#define RC_ERR_NUL_PTR  0


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
    float latitude, longitude, altitude, orientation;
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