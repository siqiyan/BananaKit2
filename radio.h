#ifndef __RADIO_H__
#define __RADIO_H__

#define RADIO_BUF_SIZE 128

#define DURATION_LOWER 1
#define DURATION_UPPER 3

typedef struct _radio_struct {
    uint8_t buf[RADIO_BUF_SIZE];
    int16_t bit_count;
    uint8_t rx_pin;
    uint8_t tx_pin;
    uint32_t time_counter;
    uint32_t last_counter;
} radio_struct_t;

#define RADIO_SUCCESS 1
#define RADIO_ERR_NUL_PTR 0

radio_struct_t *radio_create(uint8_t rx_pin, uint8_t tx_pin);
int radio_destroy(radio_struct_t *radio);
int radio_listen(radio_struct_t *radio);
int radio_play(radio_struct_t *radio);

#endif
