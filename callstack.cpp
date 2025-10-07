#include <string.h>
#include <stdlib.h>
#include "callstack.h"

void init_callstack(callstack_t *callstack) {
    int i;
    for(i = 0; i < CALLSTACK_SZ; i++) {
        callstack->stack[i] = NULL;
    }
    callstack->count = 0;
}

node_t *get_current_node(const callstack_t *callstack) {
    if(callstack->count == 0) {
        return NULL;
    } else {
        return callstack->stack[callstack->count - 1];
    }
}

int callstack_update(callstack_t *callstack) {
    node_t *node = get_current_node(callstack);
    if(node == NULL) {
        return CS_ERR_EMPTY;
    }

    switch(node->status) {
        case NODE_STARTING:
            if(node->on_init != NULL) {
                node->on_init();
            }
            node->status = NODE_RUNNING;
            break;

        case NODE_RUNNING:
            if(node->on_update != NULL) {
                node->status = node->on_update();
            }
            break;

        case NODE_RESUMING:
            if(node->on_resume != NULL) {
                node->on_resume();
            }
            node->status = NODE_RUNNING;
            break;

        case NODE_EXITING:
            if(node->on_exit != NULL) {
                node->on_exit();
            }
            pop_callstack(callstack);

            // Put the previous node (if exist) into resuming state:
            if(callstack->count > 0) {
                callstack->stack[callstack->count - 1]->status = NODE_RESUMING;
            }
            break;

        default:
            break;
    }
}

int push_callstack(callstack_t *callstack, node_t *node) {
    if(callstack->count >= CALLSTACK_SZ) {
        return CS_ERR_FULL;
    }
    if(node == NULL) {
        return CS_ERR_NUL_PTR;
    }

    node->status = NODE_STARTING;
    callstack->stack[callstack->count] = node;
    callstack->count++;
    return CS_SUCCESS;
}

int pop_callstack(callstack_t *callstack) {
    if(callstack->count == 0) {
        return CS_ERR_EMPTY;
    }
    callstack->stack[callstack->count - 1] = NULL;
    callstack->count--;
    return CS_SUCCESS;
}

node_t *create_node(
    const char *node_name,
    void (*on_init)(void),
    node_status_t (*on_update)(void),
    void (*on_resume)(void),
    void (*on_exit)(void)
) {
    node_t *node;
    node = (node_t *) malloc(sizeof(node_t));
    if(node == NULL) {
        return NULL;
    }

    strncpy(node->name, node_name, NODE_NAME_SZ);
    node->status = NODE_STARTING;
    node->on_init = on_init;
    node->on_update = on_update;
    node->on_resume = on_resume;
    node->on_exit = on_exit;
    return node;
}

int destroy_node(node_t *node) {
    if(node != NULL) {
        free(node);
        return CS_SUCCESS;
    } else {
        return CS_ERR_NUL_PTR;
    }
}