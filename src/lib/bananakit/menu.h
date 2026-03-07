#ifndef __MENU__
#define __MENU__

#include <stdint.h>

#define MENU_MAX_SZ 64

typedef struct __menu__{
    void *entry_list[MENU_MAX_SZ];
    int16_t entry_count;
    int16_t entry_index;
} menu_t;

// Return values for the API functions (signed integer):
#define MN_SUCCESS          1
#define MN_ERR_NUL_PTR      0
#define MN_ERR_FULL         -1
#define MN_ERR_EMPTY        -2

menu_t *create_menu(void);
int destroy_menu(menu_t *menu);
int menu_add_new_entry(void *new_entry, menu_t *menu);
void menu_move_up(menu_t *menu);
void menu_move_down(menu_t *menu);
void *menu_get_select(const menu_t *menu);
int menu_full(const menu_t *menu);
int menu_empty(const menu_t *menu);

#endif