#ifndef __BANANAKIT_MISC__
#define __BANANAKIT_MISC__

#include <stdint.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"

#define HEX_BUF_SIZE 32

// System IO interface data structures:
typedef struct __bananakit_io__{
    char lcd_buf0[LCD_BUF_SIZE];
    char lcd_buf1[LCD_BUF_SIZE];
    char lcd_buf2[LCD_BUF_SIZE];
    char lcd_buf3[LCD_BUF_SIZE];
    uint8_t lcd_show_needed;
    unsigned long keypress;
} bananakit_io_t;


// Return values for the API functions (signed integer):
#define BK_SUCCESS          1
#define BK_ERR_NUL_PTR      0
#define BK_ERR_FULL         -1
#define BK_ERR_EMPTY        -2
#define BK_ERR_OTHER        -3

int init_io(bananakit_io_t *io);
int register_new_node(
    const char *node_name,
    void (*on_init)(void),
    node_status_t (*on_update)(void),
    void (*on_resume)(void),
    void (*on_exit)(void),
    menu_t *menu
);
int register_new_unit(uint16_t new_unit_id, menu_t *menu);
int bytes2hex_str(uint8_t *input, int input_len, char *output, int output_limit);
int float2str(float value, char *buf, int sz, int precision);

#endif