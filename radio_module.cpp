#include <stdio.h>
#include <fs_thin.h>
#include <EEPROM.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"
#include "radio_module.h"
#include "radio.h"
#include "signal_process.h"


extern callstack_t Callstack;
extern bananakit_io_t IO;
menu_t *Radio_menu;
menu_t *Play_menu;
radio_struct_t *Radio;


void radio_module_interrupt(void);
void radio_module_menu_refresh(void);
node_status_t wait_and_return(void);
void record_init(void);
node_status_t record_update(void);
void play_init(void);
node_status_t play_update(void);
void play_exit(void);
void play_menu_refresh(void);
void delete_init(void);
node_status_t delete_update(void);
void usage_init(void);
void format_init(void);
node_status_t format_update(void);
static int do_load_record(uint16_t unit_id, radio_struct_t *radio);
static void do_save_record(void);
static void do_play_record(void);
static void do_delete_record(void);


void radio_module_init(void) {
    int fs_status;

    // Initialize file system for EEPROM:
    fs_status = thin_init_fs();
    if(fs_status == THIN_ERR_FS_NOT_FOUND) {
        if(thin_create_fs() != THIN_SUCCESS) {
            snprintf(
                IO.lcd_buf1,
                LCD_BUF_SIZE,
                "FS create fail"
            );
            return;
        }
    } else if(fs_status != THIN_SUCCESS) {
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "FS init fail"
        );
        return;
    }

    // Initialize radio module:
    Radio = radio_create(RADIO_RX_PIN, RADIO_TX_PIN);

    // Create submenu for the radio module:
    Radio_menu = create_menu();
    if(Radio_menu == NULL) {
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "Radio menu init fail"
        );
        IO.lcd_show_needed = 1;
    } else {
        register_new_node(
            "record",
            record_init,
            record_update,
            NULL,
            NULL,
            Radio_menu
        );
        register_new_node(
            "play",
            play_init,
            play_update,
            NULL,
            NULL,
            Radio_menu
        );
        register_new_node(
            "usage",
            usage_init,
            wait_and_return,
            NULL,
            NULL,
            Radio_menu
        );
        register_new_node(
            "format",
            format_init,
            format_update,
            NULL,
            NULL,
            Radio_menu
        );
        radio_module_menu_refresh();
    }

    IO.interrupt_callback = radio_module_interrupt;

    Play_menu = NULL; // Play menu is not initialize so set to NULL
}

node_status_t radio_module_update(void) {
    node_status_t next_status = NODE_RUNNING;

    if(Radio_menu == NULL) {
        return NODE_EXITING;
    }

    switch(IO.keypress) {
        case PIC_2:
            menu_move_up(Radio_menu);
            radio_module_menu_refresh();
            break;
        
        case PIC_8:
            menu_move_down(Radio_menu);
            radio_module_menu_refresh();
            break;

        case PIC_4:
            next_status = NODE_EXITING;
            break;

        case PIC_6:
            if(!menu_empty(Radio_menu)) {
                push_callstack(&Callstack, (node_t *) menu_get_select(Radio_menu));
            }
            break;

        default:
            break;
    }

    return next_status;
}

void radio_module_resume(void) {
    radio_module_menu_refresh();
}

void radio_module_exit(void) {
    destroy_menu(Radio_menu);
    radio_destroy(Radio);
    Radio_menu = NULL;
    Play_menu = NULL;
    Radio = NULL;
    IO.interrupt_callback = NULL;
}

void radio_module_interrupt(void) {
    if(Radio != NULL) {
        Radio->time_counter++;
    }
}

void radio_module_menu_refresh(void) {
    node_t *node;

    snprintf(
        IO.lcd_buf0,
        LCD_BUF_SIZE,
        "Radio Module"
    );

    if(Radio_menu != NULL && (!menu_empty(Radio_menu))) {
        node = (node_t *) menu_get_select(Radio_menu);
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "<%s>",
            node->name
        );
    }

    IO.lcd_show_needed = 1;
}

node_status_t wait_and_return(void) {
    node_status_t next_status = NODE_RUNNING;

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    return next_status;
}

/////////////////////////////////////////
// Recording functions:
void record_init(void) {
    int max_attempts;
    int listen_retval;
    int signal_found;
    int bit_start, bit_end;

    max_attempts = 1;
    signal_found = 0;

    while(max_attempts > 0) {

        listen_retval = radio_listen(Radio);
        // listen_retval = 0;
        if(listen_retval != 0) {
            // Error found, stop listen:
            break;
        }

        // Detect signal in buffer:
        signal_found = detect_signal(
            Radio->buf,
            Radio->bit_count,
            &bit_start,
            &bit_end,
            0.5, // start frequency threshold
            0.0, // stop frequency threshold
            10   // back_offset
        );
        if(signal_found) {
            // Signal processing:
            clip_signal(
                Radio->buf,
                bit_start,
                bit_end - bit_start
            );
            strip_signal(Radio->buf, Radio->bit_count);

            // Stop listen:
            break;
        }

        max_attempts--;
    }

    if((listen_retval == 0) && signal_found) {

        // No error and found signal:
        bytes2hex_str(Radio->buf, LCD_BUF_SIZE, IO.lcd_buf0, HEX_BUF_SIZE);

    } else if((listen_retval == 0) && (!signal_found)) {

        // No error and no signal:
        snprintf(
            IO.lcd_buf0,
            LCD_BUF_SIZE,
            "No signal"
        );

    } else if(listen_retval != 0) {

        // Error in radio_listen():
        snprintf(
            IO.lcd_buf0,
            LCD_BUF_SIZE,
            "listen fail:%d",
            listen_retval
        );

    }

    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "Save[4] Return[6]"
    );

    IO.lcd_show_needed = 1;
}

node_status_t record_update(void) {
    node_status_t next_status = NODE_RUNNING;

    switch(IO.keypress) {
        case PIC_4:
            // Save the recorded data:
            do_save_record();
            next_status = NODE_EXITING;
            break;

        case PIC_6:
            // Return to previous node:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    return next_status;
}

/////////////////////////////////////////////////////////////////////
// Play node functions:
void play_init(void) {
    int i;
    uint16_t unit_id;

    // Create the play menu:
    Play_menu = create_menu();
    if(Play_menu == NULL) {
        return;
    }
    for(i = 0; i < MAX_RECORDS; i++) {
        unit_id = (uint16_t) i;

        if(thin_exist(unit_id)) {
            register_new_unit(unit_id, Play_menu);
        }
    }

    play_menu_refresh();
}

node_status_t play_update(void) {
    node_status_t next_status = NODE_RUNNING;
    node_t *delete_node;
    uint16_t play_unit_id;

    switch(IO.keypress) {
        case PIC_2:
            menu_move_up(Play_menu);
            play_menu_refresh();
            break;

        case PIC_8:
            menu_move_down(Play_menu);
            play_menu_refresh();
            break;

        case PIC_4:
            // Exit play menu:
            next_status = NODE_EXITING;
            break;

        case PIC_6:
            // Play the selected record:
            do_play_record();
            snprintf(IO.lcd_buf1, LCD_BUF_SIZE, "Play complete");
            IO.lcd_show_needed = 1;
            break;

        case PIC_USD:
            // Delete selected record:
            delete_node = create_node(
                "delete",
                delete_init,
                delete_update,
                NULL,
                NULL
            );
            if(delete_node != NULL) {
                push_callstack(&Callstack, delete_node);
            }
            break;

        default:
            break;
    }

    return next_status;
}

void play_exit(void) {
    destroy_menu(Play_menu);
    Play_menu = NULL;
}

void play_menu_refresh(void) {
    uint16_t *unit_id;
    int i;
    
    if(Play_menu == NULL || menu_empty(Play_menu)) {
        return;
    }

    snprintf(
        IO.lcd_buf0,
        LCD_BUF_SIZE,
        "Play[6] Delete[U]"
    );

    unit_id = (uint16_t *) menu_get_select(Play_menu);
    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "record: %d",
        *unit_id
    );

    // Load the selected record into the radio buffer:
    if(do_load_record(*unit_id, Radio)) {
        // Visualize the first few bytes of the radio buffer as hex code:
        bytes2hex_str(Radio->buf, LCD_BUF_SIZE, IO.lcd_buf2, HEX_BUF_SIZE);
    } else {
        snprintf(
            IO.lcd_buf2,
            LCD_BUF_SIZE,
            "Fail to load"
        );
    }
    IO.lcd_show_needed = 1;
}

//////////////////////////////////////////////////////
// Delete node (sub-node of play node):
void delete_init(void) {
    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "Confirm del?"
    );
    snprintf(
        IO.lcd_buf2,
        LCD_BUF_SIZE,
        "Yes[4] No[6]"
    );
    IO.lcd_show_needed = 1;
}

node_status_t delete_update(void) {
    node_status_t next_status = NODE_RUNNING;

    switch(IO.keypress) {
        case PIC_4:
            // Confirm delete selected record:
            do_delete_record();
            next_status = NODE_EXITING;
            break;

        case PIC_6:
            // Do not delete:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    return next_status;
}
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
// Usage node functions:
void usage_init(void) {
    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "M:%d/%d",
        thin_get_fs_usage(),
        EEPROM.length()
    );
    IO.lcd_show_needed = 1;
}

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Format node functions:
void format_init(void) {
    snprintf(
        IO.lcd_buf1,
        LCD_BUF_SIZE,
        "Confirm Format?"
    );
    snprintf(
        IO.lcd_buf2,
        LCD_BUF_SIZE,
        "Yes[4] No[6]"
    );
    IO.lcd_show_needed = 1;
}

node_status_t format_update(void) {
    node_status_t next_status = NODE_RUNNING;

    switch(IO.keypress) {
        case PIC_4:
            // Do formatting:
            if(thin_create_fs() == THIN_SUCCESS) {
                snprintf(IO.lcd_buf1, LCD_BUF_SIZE, "FS formated");
            } else {
                snprintf(IO.lcd_buf1, LCD_BUF_SIZE, "FS format fail");
            }
            IO.lcd_show_needed = 1;

            // Return to previous node:
            next_status = NODE_EXITING;
            break;

        case PIC_6:
            // Return to previous node:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    return next_status;
}

/////////////////////////////////////////////////////////////////////
// Do functions (static):
static int do_load_record(uint16_t unit_id, radio_struct_t *radio) {
    thin_unit_t unit;
    int count;

    if(thin_exist(unit_id) == THIN_SUCCESS) {
        thin_open(unit_id, &unit);

        // Read all data until EOF is encountered and handled internally:
        thin_read(&unit, RADIO_BUF_SIZE, radio->buf, RADIO_BUF_SIZE, &count);
        radio->bit_count = count * 8;

        thin_close(&unit);
        return 1;
    } else {
        return 0;
    }
}

static void do_save_record(void) {
    int max_index;
    uint16_t new_unit_id;
    thin_unit_t new_unit;
    int unit_available;
    int thin_retval;

    if(Radio->bit_count < 0 || Radio->bit_count > 64) {
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "err bitcount:%d",
            Radio->bit_count
        );
        IO.lcd_show_needed = 1;
        return;
    }

    // Looking for an empty unit id to save data:
    unit_available = 0;
    for(new_unit_id = 0; new_unit_id < MAX_RECORDS; new_unit_id++) {

        if(thin_exist(new_unit_id) == THIN_ERR_FS_NOT_FOUND) {

            thin_retval = thin_create(new_unit_id, &new_unit);
            if(thin_retval == THIN_ERR_FS_FULL) {

                // Failed due to fs full:
                snprintf(
                    IO.lcd_buf1,
                    LCD_BUF_SIZE,
                    "err fs full"
                );

            } else if(thin_retval == THIN_SUCCESS) {

                // Create new unit success:
                unit_available = 1;

            } else {

                // Failed for other reasons:
                snprintf(
                    IO.lcd_buf1,
                    LCD_BUF_SIZE,
                    "err fs"
                );

            }

            break;
        }
    }

    if(!unit_available) {
        IO.lcd_show_needed = 1;
        return;
    }

    // Determine the buffer size:
    max_index = Radio->bit_count / 8;
    if((Radio->bit_count % 8) > 0) {
        max_index++;
    }

    // Save the signal segment data into EEPROM:
    if(thin_write(&new_unit, Radio->buf, max_index) != THIN_SUCCESS) {

        // Save failed:
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "err write"
        );

    } else {

        // Save success:
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "unit:%d saved",
            new_unit_id
        );

    }

    thin_close(&new_unit);
    IO.lcd_show_needed = 1;
}

static void do_play_record(void) {
    uint16_t *unit_id;

    if(Play_menu == NULL || menu_empty(Play_menu)) {
        return;
    }

    unit_id = (uint16_t *) menu_get_select(Play_menu);
    do_load_record(*unit_id, Radio);
    radio_play(Radio);
}

static void do_delete_record(void) {
    uint16_t *delete_unit_id;

    if(Play_menu == NULL || menu_empty(Play_menu)) {
        return;
    }

    delete_unit_id = (uint16_t *) menu_get_select(Play_menu);

    if(thin_exist(*delete_unit_id) == THIN_SUCCESS) {
        thin_delete(*delete_unit_id);
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "%d deleted",
            *delete_unit_id
        );
    } else {
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "%d not exist",
            *delete_unit_id
        );
    }

    IO.lcd_show_needed = 1;
}