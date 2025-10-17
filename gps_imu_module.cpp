#include <string.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"
#include "gps_imu_module.h"


extern callstack_t Callstack;
extern bananakit_io_t IO;


void gps_imu_module_init(void) {

}

node_status_t gps_imu_module_update(void) {
    return NODE_RUNNING;
}

void gps_imu_module_resume(void) {

}

void gps_imu_module_exit(void) {

}