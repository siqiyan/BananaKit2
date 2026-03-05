#ifndef __SIGNAL_PROCESS__
#define __SIGNAL_PROCESS__

#include <stdint.h>

#define RX_WIN_SIZE 20

int detect_signal(
    const uint8_t *buf,
    int16_t bit_count,
    int *bit_start_ptr,
    int *bit_end_ptr,
    float p_start,
    float p_stop,
    int16_t back_offset
);

int16_t clip_signal(uint8_t *buf, int16_t clip_bit_start, int16_t clip_bit_sz);

int16_t strip_signal(uint8_t *buf, int16_t bit_count);

int16_t encode_manchester(const uint8_t *input_buf, uint8_t *output_buf, int input_bit_count, int output_buf_sz);

#endif