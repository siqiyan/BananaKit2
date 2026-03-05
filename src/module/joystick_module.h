#ifndef __JOYSTICK__
#define __JOYSTICK__

#include "callstack.h"

void joystick_init(void);
node_status_t joystick_update(void);
void joystick_resume(void);
void joystick_exit(void);

#endif