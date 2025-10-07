#include <string.h>
#include <stdlib.h>
#include <util/atomic.h>
#include <Arduino.h>
#include "radio.h"

int radio_create(radio_struct_t **radio, uint8_t rx_pin, uint8_t tx_pin) {
    int i;
    radio_struct_t *radio_ptr;

    radio_ptr = (radio_struct_t *) malloc(sizeof(radio_struct_t));
    if(radio_ptr == NULL) {
        return RADIO_ERR_NUL_PTR;
    }

    radio_ptr->bit_count = 0;
    radio_ptr->rx_pin = rx_pin;
    radio_ptr->tx_pin = tx_pin;
    radio_ptr->time_counter = 0;
    radio_ptr->last_counter = 0;

    for(i = 0; i < RADIO_BUF_SIZE; i++) {
        radio_ptr->buf[i] = 0;
    }

    // Receiver set to input mode and disable internal pull-up resistor:
    pinMode(radio_ptr->rx_pin, INPUT);
    digitalWrite(radio_ptr->rx_pin, LOW);

    // Transmitter:
    pinMode(radio_ptr->tx_pin, OUTPUT);
    digitalWrite(radio_ptr->tx_pin, LOW);

    *radio = radio_ptr;
    return RADIO_SUCCESS;
}

int radio_destroy(radio_struct_t *radio) {
    if(radio != NULL) {
        free(radio);
        return RADIO_SUCCESS;
    } else {
        return RADIO_ERR_NUL_PTR;
    }
}

// void radio_cleanup(radio_struct_t *radio) {
//     int i;
//     for(i = 0; i < RADIO_BUF_SIZE; i++) {
//         radio->buf[i] = 0;
//     }
// }

int radio_listen(radio_struct_t *radio) {
    int duration;
    uint8_t is_update;
    uint8_t recv_value;
    uint16_t time_counter;
    uint16_t last_counter;
    int16_t bit_idx = 0;

    radio->bit_count = 0;
    //radio->last_counter = radio->time_counter;

    // Start counting:
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        radio->time_counter = 0;
        last_counter = 0;
    }

    while(1) {

        is_update = 0;

        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            time_counter = radio->time_counter;
        }
        // time_counter = radio->time_counter;

        duration = time_counter - last_counter;

        //duration = (radio->time_counter % 256) - (radio->last_counter % 256);
        // if(time_counter < last_counter) {
        //     duration = 65535 - last_counter + time_counter;
        // } else {
        //     duration = time_counter - last_counter;
        // }
        //if(duration != 0) {
            //return duration;
        //}
  
        if(duration >= DURATION_LOWER && duration < DURATION_UPPER) {
            is_update = 1;
            //radio->last_counter = radio->time_counter;
            last_counter = time_counter;
        } else if(duration >= DURATION_UPPER) {
            // Error: processing time exceed limit
            return duration;
        }


        if(is_update) {
            if(digitalRead(radio->rx_pin) == HIGH) {
                recv_value = 1;
            } else {
                recv_value = 0;
            }

            radio->buf[bit_idx / 8] |= recv_value << (bit_idx % 8);
            bit_idx++;

            if(bit_idx == RADIO_BUF_SIZE * 8) {
                // Radio_buf is full, stop capture:
                radio->bit_count = bit_idx;
                break;
            }
        }

    }
    
    return 0;
}

int radio_play(radio_struct_t *radio) {
    int duration;
    uint8_t is_update;
    int16_t bit_idx = 0;

    radio->last_counter = radio->time_counter;

    while(bit_idx < radio->bit_count) {

        is_update = 0;

        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            duration = (radio->time_counter % 256) - (radio->last_counter % 256);
            if(duration >= DURATION_LOWER && duration < DURATION_UPPER) {
                is_update = 1;
                radio->last_counter = radio->time_counter;
            } else if(duration >= DURATION_UPPER) {
                // Error: processing time exceed limit
                return duration;
            }
        }

        if(is_update) {
            if(radio->buf[bit_idx / 8] & (1 << (bit_idx % 8)) == 0) {
                digitalWrite(radio->tx_pin, LOW);
            } else {
                digitalWrite(radio->tx_pin, HIGH);
            }

            bit_idx++;
        }

    }

    return 0;
}

void radio_interrupt_handler(radio_struct_t *radio) {
    radio->time_counter++;
}
