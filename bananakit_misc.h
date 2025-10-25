#ifndef __BANANAKIT_MISC__
#define __BANANAKIT_MISC__

#include <stdint.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"

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

// Utility macros:
#define IS_DIGIT(x) (x >= '0' && x <= '9')
#define TO_DIGIT(x) (x - '0')
#define TO_ASCII(x) (x + '0')

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
int float2str(double value, char *buf, int sz, int precision);
int str2float(const char *buf, int bufsz, double *output);
int field_extract(
    const char *buf,
    int bufsz,
    char sep,
    int field_index,
    char *field,
    int field_sz
);
// float gps_atof(const char *buf, int bufsz, uint8_t format);
// int gps_atof(const char *buf, int bufsz, uint8_t format, float *output);
int gps_atof(const char *buf, int bufsz, int ndd, double *output);
uint8_t compute_checksum(char *frame, size_t framesize);

#endif