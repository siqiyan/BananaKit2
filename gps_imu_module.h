#ifndef __GPS_IMU_MODULE__
#define __GPS_IMU_MODULE__

#include "callstack.h"

void gps_imu_module_init(void);
node_status_t gps_imu_module_update(void);
void gps_imu_module_resume(void);
void gps_imu_module_exit(void);

#endif