#ifndef __BANANAKIT_IO__
#define __BANANAKIT_IO__
#include <stdint.h>

#define LCD_NO_REFRESH          0xFF
#define LCD_REFRESH_LINE0       0x00
#define LCD_REFRESH_LINE1       0x01
#define LCD_REFRESH_LINE2       0x02
#define LCD_REFRESH_LINE3       0x03

#define LCD_BUF_SIZE            21

#define IO_SUCCESS              1
#define IO_ERR_NUL_PTR          0


// System IO interface data structures:
typedef struct __bananakit_io__{
    char lcd_buf[LCD_BUF_SIZE];
    uint8_t flags;
    unsigned long keypress;
    void (*interrupt_callback)(void);
    void (*lcd_refresh_callback)(void);
    void (*lcd_clear_callback)(void);
} bananakit_io_t;

int init_io(
    bananakit_io_t *io,
    void (*lcd_refresh_callback) (void),
    void (*lcd_clear_callback)(void)
);

#endif