#ifndef __DC_MOTOR_MODULE__
#define __DC_MOTOR_MODULE__

#include "lib/bananakit/callstack.h"

void dc_motor_init(void);
node_status_t dc_motor_update(void);
void dc_motor_resume(void);
void dc_motor_exit(void);

#endif
