#ifndef __WEATHER_MODULE__
#define __WEATHER_MODULE__

#include "lib/bananakit/callstack.h"

void weather_module_init(void);
node_status_t weather_module_update(void);
void weather_module_resume(void);
void weather_module_exit(void);

#endif
