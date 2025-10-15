#include <stdint.h>

#include "rc_vehicle_common.h"


void init_status_code(status_code_t *code) {
    code->estop = 0;
    code->navigate = 0;
    code->navigate_reply = 0;
    code->sync_with_laptop = 0;
    code->sync_with_vehicle = 0;
}

void init_v2s_frame(v2s_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    init_status_code(&frame->status_code);
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->latitude_int = 0;
    frame->longitude_int = 0;
    frame->orientation_int = 0;
    frame->adc_value = 0;
    frame->sm_state = 0;
}

void init_s2v_frame(s2v_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    init_status_code(&frame->status_code);
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->twist_x = 0;
    frame->twist_y = 0;
    frame->twist_yaw = 0;
    frame->goal_latitude_int = 0;
    frame->goal_longitude_int = 0;
    frame->goal_orientation_int = 0;
}

void init_s2l_frame(s2l_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    init_status_code(&frame->status_code);
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->adc_value = 0;
    frame->latitude_int = 0;
    frame->longitude_int = 0;
    frame->orientation_int = 0;
    frame->sm_state = 0;
    frame->checksum = 0;
}

void init_l2s_frame(l2s_frame_t *frame) {
    frame->header = FRAME_HEADER_ID;
    init_status_code(&frame->status_code);
    frame->timestamp = 0;
    frame->sequence_id = 0;
    frame->goal_latitude_int = 0;
    frame->goal_longitude_int = 0;
    frame->goal_orientation_int = 0;
    frame->checksum = 0;
}

float compute_batt_volt(int16_t adc_value) {
    // https://lastminuteengineers.com/voltage-sensor-arduino-tutorial/

    float adc_voltage = ((float) adc_value * ADC_REF_VOLT) / 1024.0;
    return adc_voltage * (ADC_R1 + ADC_R2) / ADC_R2;
}