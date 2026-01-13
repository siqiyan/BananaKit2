// Banana string process extension library:
#ifndef __BANANA_STRING__
#define __BANANA_STRING__

#include <stdint.h>

#define IS_DIGIT(x) (x >= '0' && x <= '9')
#define TO_DIGIT(x) (x - '0')
#define TO_ASCII(x) (x + '0')

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
void right_align_overlay(const char *input, char *dest, size_t dest_len);

#endif