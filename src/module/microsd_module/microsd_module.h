#ifndef __MICROSD_MODULE__
#define __MICROSD_MODULE__

#include "lib/bananakit/callstack.h"

void microsd_module_init(void);
node_status_t microsd_module_update(void);
void microsd_module_resume(void);
void microsd_module_exit(void);

#endif
