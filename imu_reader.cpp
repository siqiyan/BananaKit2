// Standard libraries:
#include <stdlib.h>

// From MPU6050 library:
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

// Custom header file:
#include "imu_reader.h"

/* MPU6050 default I2C address is 0x68*/
MPU6050 MPU;
//MPU6050 MPU(0x69); //Use for AD0 high
//MPU6050 MPU(0x68, &Wire1); //Use for AD0 low, but 2nd Wire (TWI/I2C) object.

imu_reader_t *create_imu_reader(void) {
//   #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
//     Wire.begin();
//     Wire.setClock(400000); // 400kHz I2C clock. Comment on this line if having compilation difficulties
//   #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
//     Fastwire::setup(400, true);
//   #endif

    imu_reader_t *imu;

    imu = (imu_reader_t *) malloc(sizeof(imu_reader_t));
    if(imu == NULL) {
        return NULL;
    }

    imu->debug_code0 = 0;
    imu->debug_code1 = 0;

    imu->q.w = 1.0;
    imu->q.x = 0.0;
    imu->q.y = 0.0;
    imu->q.z = 0.0;
    imu->aa.x = 0;
    imu->aa.y = 0;
    imu->aa.z = 0;
    imu->gg.x = 0;
    imu->gg.y = 0;
    imu->gg.z = 0;
    imu->aa_world.x = 0;
    imu->aa_world.y = 0;
    imu->aa_world.z = 0;
    imu->gg_world.x = 0;
    imu->gg_world.y = 0;
    imu->gg_world.z = 0;
    imu->gravity.x = 0.0;
    imu->gravity.y = 0.0;
    imu->gravity.z = 0.0;
    // imu->euler[0] = 0.0;
    // imu->euler[1] = 0.0;
    // imu->euler[2] = 0.0;
    imu->ypr[0] = 0.0;
    imu->ypr[1] = 0.0;
    imu->ypr[2] = 0.0;

    MPU.initialize();
    if(MPU.testConnection() == false) {
        imu->debug_code0 = -1;
        return imu;
    }

    // Initializate and configure the DMP:
    imu->dev_status = MPU.dmpInitialize();

    // Supply your gyro offsets here, scaled for min sensitivity:
    MPU.setXGyroOffset(0);
    MPU.setYGyroOffset(0);
    MPU.setZGyroOffset(0);
    MPU.setXAccelOffset(0);
    MPU.setYAccelOffset(0);
    MPU.setZAccelOffset(0);

    // Making sure it worked (returns 0 if so):
    if(imu->dev_status == 0) {
        MPU.CalibrateAccel(6);  // Calibration Time: generate offsets and calibrate our MPU6050
        MPU.CalibrateGyro(6);
        // MPU.PrintActiveOffsets();
        
        //Turning ON DMP:
        MPU.setDMPEnabled(true);
        imu->mpu_int_status = MPU.getIntStatus();

        // Set the DMP Ready flag so the main loop() function knows it is okay to use it:
        imu->dmp_ready = 1;
        imu->packet_size = MPU.dmpGetFIFOPacketSize(); //Get expected DMP packet size for later comparison
    }

    return imu;
}

int destroy_imu_reader(imu_reader_t *imu) {
    if(imu == NULL) {
        return 0;
    }

    free(imu);
    return 1;
}

int imu_update(imu_reader_t *imu) {
    if(imu == NULL) {
        return 0;
    }

    if(!imu->dmp_ready) {
        imu->debug_code0 = -1;
        return -1;
    }

    if(MPU.dmpGetCurrentFIFOPacket(imu->fifo_buffer)) { // Get the Latest packet 
        MPU.dmpGetQuaternion(&imu->q, imu->fifo_buffer);
        // Serial.print("quat\t");
        // Serial.print(q.w);
        // Serial.print("\t");
        // Serial.print(q.x);
        // Serial.print("\t");
        // Serial.print(q.y);
        // Serial.print("\t");
        // Serial.println(q.z);

        MPU.dmpGetGravity(&imu->gravity, &imu->q);

        // Display initial world-frame acceleration, adjusted to remove gravity
        // and rotated based on known orientation from Quaternion:
        MPU.dmpGetAccel(&imu->aa, imu->fifo_buffer);
        MPU.dmpConvertToWorldFrame(&imu->aa_world, &imu->aa, &imu->q);
        // Serial.print(aaWorld.x * MPU.get_acce_resolution() * EARTH_GRAVITY_MS2);
        // Serial.print(aaWorld.y * MPU.get_acce_resolution() * EARTH_GRAVITY_MS2);
        // Serial.println(aaWorld.z * MPU.get_acce_resolution() * EARTH_GRAVITY_MS2);

        /* Display initial world-frame acceleration, adjusted to remove gravity
        and rotated based on known orientation from Quaternion */
        MPU.dmpGetGyro(&imu->gg, imu->fifo_buffer);
        MPU.dmpConvertToWorldFrame(&imu->gg_world, &imu->gg, &imu->q);
        // Serial.print("ggWorld\t");
        // Serial.print(ggWorld.x * MPU.get_gyro_resolution() * DEG_TO_RAD);
        // Serial.print("\t");
        // Serial.print(ggWorld.y * MPU.get_gyro_resolution() * DEG_TO_RAD);
        // Serial.print("\t");
        // Serial.println(ggWorld.z * MPU.get_gyro_resolution() * DEG_TO_RAD);

        /* Display Euler angles in degrees */
        MPU.dmpGetYawPitchRoll(imu->ypr, &imu->q, &imu->gravity);
        // Serial.print("ypr\t");
        // Serial.print(ypr[0] * RAD_TO_DEG);
        // Serial.print("\t");
        // Serial.print(ypr[1] * RAD_TO_DEG);
        // Serial.print("\t");
        // Serial.println(ypr[2] * RAD_TO_DEG);

        imu->debug_code0 = 1;
        return 1;
    } else {
        imu->debug_code0 = -2;
        return -2;
    }
}
