#include <stdint.h>
#include <string.h>
#include <CRC.h>

#include "bananakit.h"
#include "bananakit_misc.h"
#include "callstack.h"
#include "menu.h"

int register_new_node(
    const char *node_name,
    void (*on_init)(void),
    node_status_t (*on_update)(void),
    void (*on_resume)(void),
    void (*on_exit)(void),
    menu_t *menu
) {
    node_t *node;

    if(menu == NULL) {
        return BK_ERR_NUL_PTR;
    }
    if(menu_full(menu)) {
        return BK_ERR_FULL;
    }

    node = create_node(
        node_name,
        on_init,
        on_update,
        on_resume,
        on_exit
    );
    if(node == NULL) {
        return BK_ERR_NUL_PTR;
    }

    if(menu_add_new_entry((void *) node, menu) == MN_SUCCESS) {
        return BK_SUCCESS;
    } else {
        return BK_ERR_OTHER;
    }
}

int register_new_unit(uint16_t new_unit_id, menu_t *menu) {
    uint16_t *unit_id_ptr;

    if(menu == NULL) {
        return BK_ERR_NUL_PTR;
    }
    if(menu_full(menu)) {
        return BK_ERR_FULL;
    }

    unit_id_ptr = (uint16_t *) malloc(sizeof(uint16_t));
    if(unit_id_ptr == NULL) {
        return BK_ERR_NUL_PTR;
    }
    *unit_id_ptr = new_unit_id;
    
    if(menu_add_new_entry((void *) unit_id_ptr, menu) == MN_SUCCESS) {
        return BK_SUCCESS;
    } else {
        return BK_ERR_OTHER;
    }
}

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

int float2str(float value, char *buf, int sz, int precision) {
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
  int ivalue; // for integer part of the input value
  float fvalue; // float point part
  int idigit; // for counting digits

  // Count how many digits integer part has:
  ivalue = (int) value;
  idigit = 0;
  while(ivalue > 0) {
    idigit++;
    ivalue /= 10;
  }

  // Put a minus sign for negative number:
  if(value < 0) {
    *ptr++ = '-';
  }

  if(idigit == 0) {
    // Put a zero if no integer part:
    *ptr++ = '0';
  } else {
    // Convert all digits from integer part (reverse the loop since lowest
    // digit go first):
    ivalue = (int) value;
    for(int i = idigit - 1; i >= 0; i--) {
      *(ptr + i) = (ivalue % 10) + '0';
      ivalue /= 10;
    }
    ptr += idigit; // pointer move to the next available byte
  }

  *ptr++ = '.'; // put a dot for decimal digits

  // Convert all float point digits to string for given precision:
  if(value < 0) {
    fvalue = -value;
  } else {
    fvalue = value;
  }
  for(int i = 0; i < precision; i++) {
    fvalue *= 10;
    *ptr++ = ((int) fvalue) % 10 + '0';
  }

  *ptr++ = '\0'; // Put the termination mark at the end of buffer

  return ptr - buf;
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
        i < bufsz, j < field_sz - 1, field_index_count <= field_index;
        i++
    ) {
        if(field_index == field_index_count) {
            field[j++] = buf[i];
        }
        if(buf[i] == sep) {
            field_index_count++;
        }
    }

    if(j >= 0) {
        field[j] = '\0'; // replace the last character with NUL
    }

    return j;
}

float gps_atof(const char *buf, int bufsz, uint8_t format) {
    char degree[4], minute[11];

    if(format == 0) {
        // Format 0: DDMM.MMMMMMM

        // strncpy(lat, p, strchr(p, ',') - p);
        // strncpy(degree, lat, 2);
        // strcpy(minute, lat+2);
        // gnss->lat = atof(degree) + atof(minute) / 60.0;
        degree[0] = buf[0];
        degree[1] = buf[1];
        degree[2] = '\0';
        strncpy(minute, buf+2, 12);
        return atof(degree) + atof(minute) / 60.0;
    } else if(format == 1) {
        // Format 1: DDDMM.MMMMMMM

        // strncpy(lon, p, strchr(p, ',') - p);
        // strncpy(degree, lon, 3);
        // strcpy(minute, lon+3);
        // gnss->lon = atof(degree) + atof(minute) / 60.0;
        degree[0] = buf[0];
        degree[1] = buf[1];
        degree[2] = buf[2];
        strncpy(minute, buf+3, 13);
        return atof(degree) + atof(minute) / 60.0;
    }
}

// CRC8 checksum:
uint8_t compute_checksum(char *frame, size_t framesize) {
    CRC8 crc;
    crc.restart();
    crc.add((uint8_t *)frame, framesize - sizeof(uint8_t));

    // for(size_t i = 0; i < framesize - sizeof(uint8_t); i++) {
    //   crc.add(frame[i]);
    // }
    // frame->checksum = crc.calc();
    return crc.calc();
}