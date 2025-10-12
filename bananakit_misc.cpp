#include <stdint.h>
#include <CRC.h>

#include "bananakit.h"
#include "bananakit_misc.h"
#include "callstack.h"
#include "menu.h"

int init_io(bananakit_io_t *io) {
    io->lcd_buf0[0] = '\0';
    io->lcd_buf1[0] = '\0';
    io->lcd_buf2[0] = '\0';
    io->lcd_buf3[0] = '\0';
    io->lcd_show_needed = 0;
    io->keypress = 0xFFFFFF;
    io->interrupt_callback = NULL;
    return BK_SUCCESS;
}

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