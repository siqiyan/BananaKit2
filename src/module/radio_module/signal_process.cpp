#include "signal_process.h"
#include <stdint.h>

int detect_signal(
    const uint8_t *buf,
    int16_t bit_count,
    int *bit_start_ptr,
    int *bit_end_ptr,
    float p_start,
    float p_stop,
    int16_t back_offset
) {
    // Detect signal in the radio buffer
    //
    // Arguments:
    //   bit_start_ptr - pointer to starting bit index (output)
    //   bit_end - pointer to ending bit index (output)
    //   upper - upper threshold to determine start index
    //   lower - lower threshold to determine end index
    //   back_offset - should be set approximate to half window size
    //   RX_WIN_SIZE - macro of window size
    // Returns:
    //   true - signal found
    //   false - no signal

    uint8_t rx_window[RX_WIN_SIZE];
    int rx_window_idx = 0;
    int count0 = 0;
    int count1 = 0;
    float p1 = 0.0;
    uint8_t val;
    int i, j;
    int signal_found = 0;
    int rx_bit_start = 0;
    int rx_bit_end = 0;

    for(i = 0; i < bit_count; i++) {
        // Count the frequency of 1s in a sliding window:
        val = (buf[i / 8] & (1 << (i % 8))) >> (i % 8);
        rx_window[rx_window_idx] = val;

        if(rx_window_idx == RX_WIN_SIZE - 1) {
            // Compute statistics:
            count0 = 0;
            count1 = 0;
            for(j = 0; j < RX_WIN_SIZE; j++) {
                if(rx_window[j] == 1) {
                    count1++;
                } else {
                    count0++;
                }
            }
            p1 = (float) count1 / (float) (count0 + count1);

            // Shift window to leave one space for new data:
            for(j = 0; j < RX_WIN_SIZE - 1; j++) {
                rx_window[j] = rx_window[j+1];
            }

            if((!signal_found) && p1 >= p_start) {
                rx_bit_start = i - back_offset;
                signal_found = 1;
            } else if(signal_found && p1 <= p_stop) {
                rx_bit_end = i - back_offset;
                break;
            }

        } else {
            rx_window_idx++;
        }
    }

    // Limit the index range to prevent segmentation fault:
    if(rx_bit_start < 0) {
        rx_bit_start = 0;
    }
    if(rx_bit_end < 0) {
        rx_bit_end = 0;
    }
    if(rx_bit_end >= bit_count) {
        rx_bit_end = bit_count;
    }
    if(rx_bit_start > rx_bit_end) {
        rx_bit_start = rx_bit_end;
    }

    *bit_start_ptr = rx_bit_start;
    *bit_end_ptr = rx_bit_end;

    return signal_found;
}

int16_t clip_signal(uint8_t *buf, int16_t clip_bit_start, int16_t clip_bit_sz) {
    // Clip bit-wise signal segment from certain location to the beginning of the
    // buffer (override existing data in the buffer).
    // Input:
    //  radio - radio struct
    //  clip_bit_start - bit-wise start index
    //  clip_bit_sz - bit-wise length of the segment
    // Output:
    //  New bit_count after clipping

    int i;
    int ref_bit_idx;
    uint8_t byte = 0;

    for(i = 0; i < clip_bit_sz; i++) {

        ref_bit_idx = i + clip_bit_start;
        byte |= ((buf[ref_bit_idx / 8] >> (ref_bit_idx % 8)) & 1) << (i % 8);

        if((i % 8 == 0) && i > 0) {
            buf[i / 8] = byte;
            byte = 0;
        }
    }

    return clip_bit_sz;
}

int16_t strip_signal(uint8_t *buf, int16_t bit_count) {
    // Remove the heading and ending repeated data. The data is
    // considered repeated if consicutive four bits are the same.
    // i.e. 0000 and 1111 are repeated
    // Given the input data: FFE1A93B00
    // Output will be: E1A93B
    // Input:
    //  buf - input buffer
    //  bit_count - total number of bits in the buffer
    // Output:
    //  New bit_count after the operation

    int i, j;
    uint8_t byte;
    int strip_start_idx, strip_end_idx; // index for each half-byte
    int max_index;

    strip_start_idx = 0;
    strip_end_idx = 0;
    max_index = bit_count / 4;
    if((bit_count % 4) > 0) {
        max_index++;
    }

    // Detect strip segment:
    for(i = 0; i < max_index; i++) {
        j = i / 2;
        if(i % 2 == 0) {
            byte = buf[j] & 0xF;
        } else {
            byte = (buf[j] >> 4) & 0xF;
        }

        if(byte != 0 && byte != 0xF) {
            // Non-repeated data:
            if(strip_start_idx == 0) {
                strip_start_idx = i;
            } else {
                strip_end_idx = i;
            }
        }
    }

    // Clip the segment:
    return clip_signal(buf, strip_start_idx*4, (strip_end_idx - strip_start_idx)*4);
}

int16_t encode_manchester(const uint8_t *input_buf, uint8_t *output_buf, int input_bit_count, int output_buf_sz) {
    // Encode the input buffer according to Manchester encoding
    // Manchester encoding:
    // 1: high-low
    // 0: low-high
    // Input:
    //  in_buf - input buffer
    //  out_buf - output buffer
    //  in_bit_count - number of bits in input buffer
    //  out_buf_sz - output buffer size in bytes
    // Output:
    //  Total number of bits in output buffer, if error occured then return -1

    int i, j;

    // Encode input buf and store the encoded data into output_buf:
    for(i = 0, j = 0; i < input_bit_count; i++) {
        if(j >= output_buf_sz - 1) {
            // Error: output_buf does not have enought space for next two bits
            return -1;
        }
        if((input_buf[i / 8] >> (i % 8)) & 1 == 1) {
            output_buf[j / 8] |= 1 << (j % 8);
        }
        j++;
    }
}

// void decode_manchester(const uint8_t *in_buf, uint8_t *out_buf) {
// }