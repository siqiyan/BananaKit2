#include <stdio.h>

#include "bananakit_io.h"

int init_io(
    bananakit_io_t *io,
    void (*lcd_refresh_callback) (void),
    void (*lcd_clear_callback)(void)
) {
    int i;

    for(i = 0; i < LCD_BUF_SIZE; i++) {
        io->lcd_buf[i] = '\0';
    }
    io->flags = LCD_NO_REFRESH;
    io->keypress = 0xFFFFFF;
    io->interrupt_callback = NULL;
    io->lcd_refresh_callback = lcd_refresh_callback;
    io->lcd_clear_callback = lcd_clear_callback;

    return IO_SUCCESS;
}