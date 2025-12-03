#ifndef __CALLSTACK__
#define __CALLSTACK__

#include <stdint.h>

#define NODE_NAME_SZ 16 // Max size for a node name
#define CALLSTACK_SZ 8 // Max calls allowed per stack

typedef enum __node_status__{
    NODE_STARTING,
    NODE_RUNNING,
    NODE_RESUMING,
    NODE_EXITING
} node_status_t;

typedef struct __node__{
    char name[NODE_NAME_SZ];
    void (*on_init)(void);
    node_status_t (*on_update)(void);
    void (*on_resume)(void);
    void (*on_exit)(void);
    node_status_t status;
} node_t;

typedef struct __callstack__{
    node_t *stack[CALLSTACK_SZ];
    int16_t count;
} callstack_t;

// Return values for the API functions (signed integer):
#define CS_SUCCESS          1
#define CS_ERR_NUL_PTR      0
#define CS_ERR_FULL         -1
#define CS_ERR_EMPTY        -2

// API functions:
void init_callstack(callstack_t *callstack);
// int get_callstack_pwd(const callstack_t *callstack, char *path, size_t pathsz);
node_t *get_current_node(const callstack_t *callstack);
int callstack_update(callstack_t *callstack);
int push_callstack(callstack_t *callstack, node_t *node);
int pop_callstack(callstack_t *callstack);
node_t *create_node(
    const char *node_name,
    void (*on_init)(void),
    node_status_t (*on_update)(void),
    void (*on_resume)(void),
    void (*on_exit)(void)
);
int destroy_node(node_t *node);

#endif