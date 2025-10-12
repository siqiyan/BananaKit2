#ifndef __BANANAKIT__
#define __BANANAKIT__

///////////////////////////////////////////////////////////////
// Built-in modules:
// 2004 LCD I2C module:
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#define LCD_ADDR 0x27
#define LCD_HEIGHT 4 // lcd screen height
#define LCD_WIDTH 20 // lcd screen width
#define LCD_BUF_SIZE (LCD_WIDTH + 1)

// IR receiver:
#include <IRremote.h>
#define IR_SENSOR_PIN 2
// Primary IR Controller (PIC) Keypress:
#define PIC_POWER   0xFFA25D
#define PIC_MODE    0xFF629D
#define PIC_MUTE    0xFFE21D
#define PIC_PLAY    0xFF22DD
#define PIC_JBACK   0xFF02FD
#define PIC_JFORW   0xFFC23D
#define PIC_EQ      0xFFE01F
#define PIC_MINUS   0xFFA857
#define PIC_PLUS    0xFF906F
#define PIC_0       0xFF6897
#define PIC_CHAOS   0xFF9867
#define PIC_USD     0xFFB04F
#define PIC_1       0xFF30CF
#define PIC_2       0xFF18E7
#define PIC_3       0xFF7A85
#define PIC_4       0xFF10EF
#define PIC_5       0xFF38C7
#define PIC_6       0xFF5AA5
#define PIC_7       0xFF42BD
#define PIC_8       0xFF4AB5
#define PIC_9       0xFF52AD

// Built-in potentiometer:
#define VOLT0_READ_PIN  A0
///////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////
// Removable modules:
// Radio TR module:
#define RADIO_RX_PIN 4
#define RADIO_TX_PIN 3

// MicroSD adapter module:
#define MICROSD_CS_PIN 10

// Joystick module:
#define JOY_VRX             A2
#define JOY_VRY             A3
#define JOY_SW              3
#define JOY_PUSH_BUTTON_0   6
#define JOY_PUSH_BUTTON_1   5

// nrf24 module:
#define NRF24_CE_PIN        10
#define NRF24_CSN_PIN       9
///////////////////////////////////////////////////////////////

// Module enablers:
#define ENABLE_RADIO_MODULE
// #define ENABLE_RC_STATION
// #define ENABLE_MICROSD_MODULE
#define ENABLE_JOYSTICK_MODULE

// Interruption:
#define ENABLE_INT1

#endif