#ifndef _lb_TREE_H_
#define _lb_TREE_H_

////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "pre_define.h"

////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////
#define NODE_PADDING    (0)

#define MAX_LEVEL           (100)
#define MAX_INNER_DEGREE    (50)
#define MAX_LEAF_DEGREE     (50)

#define HASH_TABLE_SIZE (200 * 1.7)


#define SEARCH              +1
#define SCAN                +2
#define ADD                 +3
#define REMOVE              +4


#define DEAD                -5
#define LOST                -4
#define ROLLBACK            -3
#define CONTINUE            -2
#define FAIL                -1
#define NOTHING              0
#define SUCCESS             +1
#define RESERVE             +2


#define MERGE_REGIST        -1
#define FREE                +0
#define SPLIT_REGIST        +1

#define NORMAL_TYPE         +0
#define HEADER_TYPE         +1
#define TAIL_TYPE           +2

#define BACKEND_TRY_LIMIT 5
////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////
// B+TREE                   //

typedef struct inner_node inner_node_t;
typedef struct leaf_array leaf_array_t;
typedef struct inner_array inner_array_t;

// <STATE>      -3 == DEAD      -2 == FREEZE SLAVE      -1 == FREEZE MASTER         +0 == FREE           +1 == REGIST MASTER      +2 == REGIST SLVAE    +3 == INFANT
// <COUNT>      -2 == TAIL      -1 == NORMAL            +0 == HEADER                


typedef struct leaf_node {
    int key;
} leaf_node_t;

typedef struct inner_node {
    int key;
    leaf_array_t *p_leaf_child;
    inner_array_t *p_inner_child;
}inner_node_t;

typedef struct leaf_array {
    int state;
    int count;
    int level;
    int key;
    leaf_node_t *p_entry;
    leaf_array_t *p_next;
}leaf_array_t;

typedef struct inner_array {
    int state;
    int count;
    int level;
    int key;
    inner_node_t *p_entry;
    inner_array_t *p_next;
}inner_array_t;

typedef struct root_node {
    inner_array_t *p_inner;
} root_node_t;

// <OPERATOR>                       +10 ==: LEAF_SPLIT      -10 ==: LEAF_MERGE

typedef struct smo {
    int operator;
    inner_array_t *p_master_inner, *p_slave_inner;
    leaf_array_t *p_master_leaf, *p_slave_leaf;
}smo_t;

////////////////////////////////////////////////////////////////
// INTERFACE
////////////////////////////////////////////////////////////////
root_node_t *rlu_new_tree();

void rlu_new_smo(multi_thread_data_t *p_data);

int rlu_lb_tree_add(multi_thread_data_t *p_data, int input, int option);
int rlu_lb_tree_remove(multi_thread_data_t *p_data, int input, int option);

int rlu_lb_tree_search(multi_thread_data_t *p_data, int input, int option);
int rlu_lb_tree_range_scan(multi_thread_data_t *p_data, int start, int end, int option);

int rlu_lb_tree_split(multi_thread_data_t *p_data, int option);
int rlu_lb_tree_merge(multi_thread_data_t *p_data, int option);

int rlu_lb_tree_print(multi_thread_data_t *p_data, int start_lv, int end_lv, int option);


#endif