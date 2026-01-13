#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "banana_string.h"

int bytes2hex_str(uint8_t *input, int input_len, char *output, int output_limit) {
    // Convert bytes to hex code for visualization:
    // Input:
    //    input - Input buffer
    //    input_len - num of bytes in input buffer
    //    output - output buffer
    //    output_limit - max size of outbuf, should be set sufficient large
    // Return:
    //    Num of characters in output buffer (include term character)
    const char table[] = "0123456789ABCDEF";
    int i, j;
    for(i = 0, j = 0; i < input_len && j < output_limit - 1; i++, j += 2) {
        output[j] = table[input[i] & 0xf]; // lsb first
        output[j+1] = table[(input[i] >> 4) & 0xf]; // msb second
    }
    output[j] = '\0';
    return j;
}

int float2str(double value, char *buf, int sz, int precision) {
    // Convert a float number into string for displaying.
    // Inputs:
    //    value - input float number
    //    buf - pointer to output buffer
    //    sz - output buffer size (TODO)
    //    precision - required number of decimals
    // Outputs:
    //    Return the number of bytes in the output buffer

    // Warning:
    // The input buf size must be sufficient large since this function does not
    // check for space availability.

    char *ptr = buf;
    int ivalue;         // for integer part of the input value
    double fvalue;      // float point part
    int digits;         // for counting digits
    int fmulti;
    int i;

    // Count how many digits integer part has:
    ivalue = abs((int) value);
    digits = 0;
    while(ivalue > 0) {
        digits++;
        ivalue /= 10;
    }

    // Put a minus sign for negative number:
    if(value < 0) {
        *ptr++ = '-';
    }

    if(digits == 0) {
        // Put a zero if no integer part:
        *ptr++ = '0';
    } else {
        // Convert all digits from integer part (reverse the loop since lowest
        // digit go first):
        ivalue = abs((int) value);
        for(i = digits - 1; i >= 0; i--) {
            ptr[i] = TO_ASCII((abs(ivalue) % 10));
            ivalue /= 10;
        }
        ptr += digits; // pointer move to the next available byte
    }

    *ptr++ = '.'; // put a dot for decimal digits

    // Convert all float point digits to string for given precision:
    if(value < 0) {
        fvalue = -value;
    } else {
        fvalue = value;
    }
    fmulti = 1;
    for(i = 0; i < precision; i++) {
        fmulti *= 10;
        // fvalue *= 10;
        *ptr++ = TO_ASCII(abs((int) (fvalue * (double) fmulti)) % 10);
    }

    *ptr++ = '\0'; // Put the termination mark at the end of buffer

    return ptr - buf;
}

int str2float(const char *buf, int bufsz, double *output) {
    // https://docs.arduino.cc/language-reference/en/variables/data-types/float/
    // The float data type has only 6-7 decimal digits of precision.
    // That means the total number of digits, not the number to the
    // right of the decimal point. Unlike other platforms, where you
    // can get more precision by using a double (e.g. up to 15 digits),
    // on the Arduino board, double is the same size as float.

    int fvalue; // float point part
    double sign;
    int ivalue; // integer part
    double fmulti; // float point part multiplier
    int i;
    uint8_t state;

    if(buf == NULL || output == NULL) {
        return -1;
    }

    if(bufsz <= 0) {
        return -2;
    }

    // Initial value:
    fvalue = 0;
    fmulti = 1.0;
    ivalue = 0;
    sign = 1.0;
    if(buf[0] == '-') {
        sign = -1.0;
    }

    // State:
    //  0 - integer part
    //  1 - float number part
    state = 0;
    for(i = 0; i < bufsz && buf[i] != '\0'; i++) {
        if(state == 0) {
            if(buf[i] == '.') {
                state = 1;
            } else if(IS_DIGIT(buf[i])) {
                // if(ivalue >= 10000) {
                //     break;
                // }
                ivalue *= 10;
                ivalue += (int) TO_DIGIT(buf[i]);
            } else {
                // Non-digit character (nor '.') character encountered:
                return -3;
            }
        } else if(state == 1) {
            if(IS_DIGIT(buf[i])) {
                // if(fvalue >= 10000) {
                //     break;
                // }
                fvalue *= 10;
                fvalue += (int) TO_DIGIT(buf[i]);
                fmulti /= 10.0;
            } else {
                // Non-digit character encountered:
                return -4;
            }
        }
    }

    *output = sign * ((double) ivalue + ((double) fvalue) * fmulti);
    // *output = ((float) fvalue) * fmulti;;
    return 1;
}

int field_extract(
    const char *buf,
    int bufsz,
    char sep,
    int field_index,
    char *field,
    int field_sz
) {
    int i, j;
    int field_index_count;

    if(buf == NULL || field == NULL || bufsz == 0 || field_sz == 0) {
        return -1;
    }

    for(
        i = 0, j = 0, field_index_count = 0;
        i < bufsz && j < field_sz - 1 && field_index_count <= field_index;
        i++
    ) {
        if(field_index == field_index_count) {
            field[j++] = buf[i];
        }
        if(buf[i] == sep) {
            field_index_count++;
        }
    }

    if(j > 0) {
        // Replace the unwanted last character (i.e. the ',') with NUL
        field[j - 1] = '\0';
    } else {
        field[0] = '\0';
    }
    return j - 1; // Return the actual character size in field buf (not include NUL)
}

/**
 * Generated by Gemini
 * Aligns 'input' to the right of 'dest'.
 * * @param input: The string to be placed on the right.
 * @param dest: The output buffer containing existing text.
 * @param dest_len: The total size of the dest buffer (including null terminator).
 */
void right_align_overlay(const char *input, char *dest, size_t dest_len) {
    if (input == NULL || dest == NULL || dest_len == 0) {
        return;
    }

    size_t input_len = strlen(input);

    // Requirement: If input is larger than buffer, do nothing.
    // We use dest_len - 1 to ensure space for the null terminator.
    if (input_len >= dest_len) {
        return;
    }

    int dest_str_sz = strlen(dest);
    // Wipe the dest buffer unused area (include '\0' character)
    memset(dest + dest_str_sz, ' ', dest_len - dest_str_sz - 1);

    // Calculate the starting index for the right-aligned string.
    // Example: Buffer size 10, Input "Hi" (len 2). 
    // Start index = 10 - 1 - 2 = 7.
    size_t start_index = dest_len - 1 - input_len;

    // Copy the input string into the calculated position.
    // memmove is used instead of strcpy to avoid touching the 
    // prefix of the buffer and to handle potential (though unlikely here) overlaps.
    // memcpy(dest + start_index, input, input_len);
    strncpy(dest + start_index, input, input_len);

    // Ensure the buffer is null-terminated at the very end.
    dest[dest_len - 1] = '\0';
}