#ifndef __BANANAKIT_MISC__
#define __BANANAKIT_MISC__

#include <stdint.h>
#include <stddef.h>
#include "callstack.h"
#include "menu.h"
// #include "bananakit.h"

#define HEX_BUF_SIZE 32

// Return values for the API functions (signed integer):
#define BK_SUCCESS          1
#define BK_ERR_NUL_PTR      0
#define BK_ERR_FULL         -1
#define BK_ERR_EMPTY        -2
#define BK_ERR_OTHER        -3
#define BK_ERR_FORMAT       -4
#define BK_ERR_FLOAT        -5
#define BK_ERR_TOO_SMALL    -6

int register_new_node(
    const char *node_name,
    void (*on_init)(void),
    node_status_t (*on_update)(void),
    void (*on_resume)(void),
    void (*on_exit)(void),
    menu_t *menu
);
int register_new_unit(uint16_t new_unit_id, menu_t *menu);
uint8_t compute_checksum(char *frame, size_t framesize);

#endif