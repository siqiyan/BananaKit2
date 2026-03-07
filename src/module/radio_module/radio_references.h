#define HALF_PULSE_DELAY_US 50
#define PULSE_DELAY_US 200
// Manchester encoding:
// 1: high-low
// 0: low-high
void transmit_rf_buffer(const char *buf, size_t bufsize, int radio_tx_pin) {
    char *ptr = buf;
    int i, j, k;
    for(i = 0; i < bufsize; i++) {
        for(j = 0; j < 8; j++) {
            // Send every bit 3 times:
            for(k = 0; k < 3; k++) {
                if((*ptr >> j) & 0x01 == 1) {
                    digitalWrite(radio_tx_pin, HIGH);
                    delayMicroseconds(PULSE_DELAY_US);
                    digitalWrite(radio_tx_pin, LOW);
                    delayMicroseconds(PULSE_DELAY_US);
                } else {
                    digitalWrite(radio_tx_pin, LOW);
                    delayMicroseconds(PULSE_DELAY_US);
                    digitalWrite(radio_tx_pin, HIGH);
                    delayMicroseconds(PULSE_DELAY_US);
                }
            }
        }
        ptr++;
    }
}
void transmit_rf_header(int radio_tx_pin) {
    for(int i = 0; i < 2; i++) {
        digitalWrite(radio_tx_pin, HIGH);
        delayMicroseconds(PULSE_DELAY_US);
        digitalWrite(radio_tx_pin, LOW);
        delayMicroseconds(PULSE_DELAY_US);
    }
    digitalWrite(radio_tx_pin, HIGH);
    delayMicroseconds(PULSE_DELAY_US);
    delayMicroseconds(PULSE_DELAY_US);
    digitalWrite(radio_tx_pin, LOW);
    delayMicroseconds(PULSE_DELAY_US);
    delayMicroseconds(PULSE_DELAY_US);
}

int receive_rf_bit(int radio_rx_pin) {
    int k;
    uint8_t bit_buffer;
    bit_buffer = 0;
    delayMicroseconds(HALF_PULSE_DELAY_US);
    for(k = 0; k < 6; k++) {
        // Each bit requires 2 * 3 = 6 read cycles
        // Where Manchester encoding takes 2 cycles, and
        // each bit send 3 times, so total = 6 cycles
        if(digitalRead(radio_rx_pin) == HIGH) {
            bit_buffer |= 1;
        }
        delayMicroseconds(PULSE_DELAY_US);
        bit_buffer <<= 1;
    }
    switch(bit_buffer) {
        case 0x2A:
        case 0x28:
        case 0x29:
        case 0x2B:
        case 0x22:
        case 0x26:
        case 0x2E:
        case 0x0A:
        case 0x1A:
        case 0x3A:
            // Result is 1
            return 1;
            break;
        case 0x15:
        case 0x14:
        case 0x16:
        case 0x17:
        case 0x11:
        case 0x19:
        case 0x1D:
        case 0x05:
        case 0x25:
        case 0x35:
            // Result is 0
            return 0;
            break;
        default:
            // Failed to decode
            return -1;
            // break;
    }
}

size_t receive_rf_buffer(char *buf, size_t bufsize, int radio_rx_pin) {
    char *ptr = buf;
    uint8_t bit_buffer;
    int i, j, k;
    size_t bit_counter = 0;

    // Wait until the half of pulse cycle and read bit:
    // delayMicroseconds(HALF_PULSE_DELAY_US);

    for(i = 0; i < bufsize; i++) {
        for(j = 0; j < 8; j++) {
            bit_buffer = 0;

            for(k = 0; k < 6; k++) {
                // Each bit requires 2 * 3 = 6 read cycles
                // Where Manchester encoding takes 2 cycles, and
                // each bit send 3 times, so total = 6 cycles
                
                if(digitalRead(radio_rx_pin) == HIGH) {
                    bit_buffer |= 1;
                }
                delayMicroseconds(PULSE_DELAY_US);
                bit_buffer <<= 1;
            }

            // Manchester decoding:
            // 1: high-low
            // 0: low-high
            // Decoding (with error correction):
            // Bin     Hex    Result    Note
            // 101010  0x2A   1         No corruption
            // 010101  0x15   0         No corruption

            // 101000  0x28   1
            // 101001  0x29   1
            // 101011  0x2B   1         Last bit corrupted

            // 100010  0x22   1
            // 100110  0x26   1
            // 101110  0x2E   1         Second bit corrupted

            // 001010  0x0A   1
            // 011010  0x1A   1
            // 111010  0x3A   1         First bit corrupted

            // 010100  0x14   0
            // 010110  0x16   0
            // 010111  0x17   0         Last bit corrupted

            // 010001  0x11   0
            // 011001  0x19   0
            // 011101  0x1D   0         Second bit corrupted

            // 000101  0x05   0
            // 100101  0x25   0
            // 110101  0x35   0         First bit corrupted

            switch(bit_buffer) {
                case 0x2A:
                case 0x28:
                case 0x29:
                case 0x2B:
                case 0x22:
                case 0x26:
                case 0x2E:
                case 0x0A:
                case 0x1A:
                case 0x3A:
                    // Result is 1
                    *ptr |= 1;
                    break;
                case 0x15:
                case 0x14:
                case 0x16:
                case 0x17:
                case 0x11:
                case 0x19:
                case 0x1D:
                case 0x05:
                case 0x25:
                case 0x35:
                    // Result is 0
                    // (No need to do anything)
                    break;
                default:
                    // Failed to decode
                    return bit_counter;
                    // break;
            }
            *ptr <<= 1;
            bit_counter++;
        }
        ptr++;
    }
    return bit_counter;
}

bool rf_repeat_read(int radio_rx_pin) {
    // Each digitalRead() takes around 4us
    int rd0, rd1, rd2;

    int high_count = 0;
    for(int i = 0; i < 10; i++) {
        if(digitalRead(radio_rx_pin) == HIGH) {
            high_count++;
        }
    }
    rd0 = digitalRead(radio_rx_pin);
    delayMicroseconds(10);
    rd1 = digitalRead(radio_rx_pin);
    delayMicroseconds(10);
    rd2 = digitalRead(radio_rx_pin);
    return (rd0 == HIGH && rd1 == HIGH) || (rd1 == HIGH && rd2 == HIGH) || (rd1 == HIGH && rd2 == HIGH);
}

#define DETECT_ATTEMPTS 20
bool detect_rf_header(int radio_rx_pin) {
    uint8_t recvbuf = 0;
    for(int i = 0; i < DETECT_ATTEMPTS; i++) {
        if(rf_repeat_read(radio_rx_pin)) {
            recvbuf |= 1;
        }
        delayMicroseconds(PULSE_DELAY_US - 20);
        if(recvbuf == 0xAC) {
            // 0xAC == 1010 1100 <- header id
            return true;
        }
        recvbuf <<= 1;
    }
    return false;
    
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