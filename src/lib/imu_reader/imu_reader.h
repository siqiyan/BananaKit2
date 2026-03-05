#ifndef __IMU_READER__
#define __IMU_READER__

#include <stdint.h>

// From MPU6050 library:
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

/*Conversion variables*/
#define EARTH_GRAVITY_MS2 9.80665  //m/s2
#define DEG_TO_RAD        0.017453292519943295769236907684886
#define RAD_TO_DEG        57.295779513082320876798154814105

typedef struct __imu_reader__ {

    int8_t debug_code0;
    int8_t debug_code1;

    /*---MPU6050 Control/Status Variables---*/
    uint8_t dmp_ready;  // Set true if DMP init was successful
    uint8_t mpu_int_status;   // Holds actual interrupt status byte from MPU
    uint8_t dev_status;      // Return status after each device operation (0 = success, !0 = error)
    uint16_t packet_size;    // Expected DMP packet size (default is 42 bytes)
    uint8_t fifo_buffer[64]; // FIFO storage buffer

    /*---MPU6050 Control/Status Variables---*/
    Quaternion q;           // [w, x, y, z]         Quaternion container
    VectorInt16 aa;         // [x, y, z]            Accel sensor measurements
    VectorInt16 gg;         // [x, y, z]            Gyro sensor measurements
    VectorInt16 aa_world;    // [x, y, z]            World-frame accel sensor measurements
    VectorInt16 gg_world;    // [x, y, z]            World-frame gyro sensor measurements
    VectorFloat gravity;    // [x, y, z]            Gravity vector
    // float euler[3];         // [psi, theta, phi]    Euler angle container
    float ypr[3];           // [yaw, pitch, roll]   Yaw/Pitch/Roll container and gravity vector

} imu_reader_t;

imu_reader_t *create_imu_reader(void);
int destroy_imu_reader(imu_reader_t *imu);
int imu_update(imu_reader_t *imu);

#endif