#include <stdint.h>
#include <CRC.h>

// #include "bananakit.h"
#include "callstack.h"
#include "menu.h"
#include "bananakit_misc.h"

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