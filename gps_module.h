#ifndef __GPS_MODULE__
#define __GPS_MODULE__

#include "callstack.h"
void gps_module_init(void);
node_status_t gps_module_update(void);
void gps_module_resume(void);
void gps_module_exit(void);

#endif