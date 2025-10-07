#include <stdlib.h>
#include "menu.h"

menu_t *create_menu(void) {
    int i;
    menu_t *menu_ptr = (menu_t *) malloc(sizeof(menu_t));
    if(menu_ptr == NULL) {
        return NULL;
    }
    for(i = 0; i < MENU_MAX_SZ; i++) {
        menu_ptr->entry_list[i] = NULL;
    }
    menu_ptr->entry_count = 0;
    menu_ptr->entry_index = 0;
    return menu_ptr;
}

int destroy_menu(menu_t *menu) {
    int i;
    if(menu != NULL) {
        for(i = 0; i < menu->entry_count; i++) {
            if(menu->entry_list[i] != NULL) {
                free(menu->entry_list[i]);
            }
        }
        free(menu);
        return MN_SUCCESS;
    } else {
        return MN_ERR_NUL_PTR;
    }
}

int menu_add_new_entry(void *new_entry, menu_t *menu) {
    if(menu == NULL) {
        return MN_ERR_NUL_PTR;
    }
    if(menu_full(menu)) {
        return MN_ERR_FULL;
    }
    menu->entry_list[menu->entry_count] = new_entry;
    menu->entry_count++;
    return MN_SUCCESS;
}

void menu_move_up(menu_t *menu) {
    menu->entry_index--;
    if(menu->entry_index < 0) {
        menu->entry_index = menu->entry_count - 1;
    }
}

void menu_move_down(menu_t *menu) {
    menu->entry_index++;
    if(menu->entry_index >= menu->entry_count) {
        menu->entry_index = 0;
    }
}

void *menu_get_select(const menu_t *menu) {
    if(menu == NULL) {
        return NULL;
    }
    if(menu->entry_count == 0) {
        return NULL;
    }
    return menu->entry_list[menu->entry_index];
}

int menu_full(const menu_t *menu) {
    return menu->entry_count >= MENU_MAX_SZ;
}

int menu_empty(const menu_t *menu) {
    return menu->entry_count == 0;
}