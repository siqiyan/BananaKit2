#ifndef __RADIO_MODULE__
#define __RADIO_MODULE__

#include "lib/bananakit/callstack.h"

#define MAX_RECORDS 20 // maximum number of signal records to keep in EEPROM

void radio_module_init(void);
node_status_t radio_module_update(void);
void radio_module_resume(void);
void radio_module_exit(void);

#endif
