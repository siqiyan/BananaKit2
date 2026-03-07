#ifndef __IMU_MODULE__
#define __IMU_MODULE__

#include "lib/bananakit/callstack.h"

void imu_module_init(void);
node_status_t imu_module_update(void);
void imu_module_resume(void);
void imu_module_exit(void);

#endif
