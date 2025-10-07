#include <stdio.h>

#include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"

#ifdef ENABLE_RADIO_MODULE
    #include "radio_module.h"
#endif

// #ifdef ENABLE_MICROSD_MODULE
//     #include <SD.h>
// #endif

LiquidCrystal_I2C Lcd(LCD_ADDR, LCD_WIDTH, LCD_HEIGHT);
IRrecv IR_recver(IR_SENSOR_PIN);
decode_results IR_results;

callstack_t Callstack;
node_t *Main_node;
menu_t *Main_menu;
bananakit_io_t IO;

node_status_t main_menu_update(void);
static void main_menu_flush(void);
void main_menu_resume(void);

void setup(void) {
    // Initialize Bananakit IO:
    IR_recver.enableIRIn();
    Lcd.init();
    Lcd.backlight();
    init_io(&IO);

    // Initialize the main node with callstack:
    init_callstack(&Callstack);
    Main_node = create_node(
        "main_node",
        NULL,
        main_menu_update,
        main_menu_resume,
        NULL
    );
    if(Main_node == NULL) {
        Lcd.setCursor(0, 1);
        Lcd.print("Main node init fail");
        while(1);
    }
    push_callstack(&Callstack, Main_node);

    // Initialize menu:
    Main_menu = create_menu();
    if(Main_menu == NULL) {
        Lcd.setCursor(0, 1);
        Lcd.print("Main menu init fail");
        while(1);
    }

#ifdef ENABLE_RADIO_MODULE
    register_new_node(
        "radio",
        radio_module_init,
        radio_module_update,
        radio_module_resume,
        radio_module_exit,
        Main_menu
    );
#endif

#ifdef EN_INT1
    // --------------------------------------------------------------------------
    // Setup timer1 interrupt:
    // Code generated from:
    // https://www.8bit-era.cz/arduino-timer-interrupts-calculator.html

    // TIMER 1 for interrupt frequency 1000 Hz:
    cli(); // stop interrupts
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1  = 0; // initialize counter value to 0
    // set compare match register for 1000 Hz increments
    OCR1A = 15999; // = 16000000 / (1 * 1000) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12, CS11 and CS10 bits for 1 prescaler
    TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
    sei(); // allow interrupts
#endif
    // --------------------------------------------------------------------------

    main_menu_flush();
}

void loop(void) {
    IO.keypress = 0xFFFFFF;
    if(IR_recver.decode(&IR_results)) {
        IO.keypress = IR_results.value;
        IR_recver.resume();
    }

    callstack_update(&Callstack);

    if(IO.lcd_show_needed) {
        Lcd.clear();
        Lcd.setCursor(0, 0);
        Lcd.print(IO.lcd_buf0);
        Lcd.setCursor(0, 1);
        Lcd.print(IO.lcd_buf1);
        Lcd.setCursor(0, 2);
        Lcd.print(IO.lcd_buf2);

        // The last row is reserved for the system:
        // get_callstack_path(&Callstack, current_path, LCD_BUF_SIZE);
        // snprintf(
        //     IO.lcd_buf3,
        //     LCD_BUF_SIZE,
        //     current_path
        // );
        // Lcd.setCursor(0, 3);
        // Lcd.print(IO.lcd_buf3);

        IO.lcd_buf0[0] = '\0';
        IO.lcd_buf1[0] = '\0';
        IO.lcd_buf2[0] = '\0';
        IO.lcd_buf3[0] = '\0';
        IO.lcd_show_needed = 0;
    }
    delay(50);
}

node_status_t main_menu_update(void) {
    node_status_t next_status = NODE_RUNNING;
    node_t *next_node;

    switch(IO.keypress) {
        case PIC_2:
            menu_move_up(Main_menu);
            IO.lcd_show_needed = 1;
            main_menu_flush();
            break;

        case PIC_8:
            menu_move_down(Main_menu);
            IO.lcd_show_needed = 1;
            main_menu_flush();
            break;

        case PIC_6:
            next_node = (node_t *) menu_get_select(Main_menu);
            next_node->status = NODE_STARTING;
            push_callstack(&Callstack, next_node);
            break;

        default:
            break;
    }

    return next_status;
}

void main_menu_resume(void) {
    main_menu_flush();
}

static void main_menu_flush(void) {
    node_t *node;

    snprintf(
        IO.lcd_buf0,
        LCD_BUF_SIZE,
        "Main Menu"
    );

    if(Main_menu != NULL && (!menu_empty(Main_menu))) {
        node = menu_get_select(Main_menu);
        snprintf(
            IO.lcd_buf1,
            LCD_BUF_SIZE,
            "<%s>",
            node->name
        );
    }
    
    IO.lcd_show_needed = 1;
}

// Timer1 interrupt handle:
ISR(TIMER1_COMPA_vect) {
    // radio_interrupt_handler(&Radio);
}
