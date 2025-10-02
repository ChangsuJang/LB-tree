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
#define MAX_INNER_DEGREE    (500)
#define MAX_LEAF_DEGREE     (500)

#define HASH_TABLE_SIZE (200 * 1.7)

#define DEAD                -5
#define LOST                -4
#define ROLLBACK            -3
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

typedef struct hash_entry {
    int key;
    unsigned long address;
} hash_entry_t;

// <STATE>      -3 == DEAD      -2 == FREEZE SLAVE      -1 == FREEZE MASTER         +0 == FREE           +1 == REGIST MASTER      +2 == REGIST SLVAE    +3 == INFANT
// <COUNT>      -2 == TAIL      -1 == NORMAL            +0 == HEADER                
typedef struct leaf_node 
leaf_node_t;
typedef struct leaf_node {
    int state;
    int count;
    int type;
    int key;
    leaf_node_t *p_next;
} leaf_node_t;

typedef struct inner_entry inner_entry_t;
typedef struct inner_node inner_node_t;

typedef struct inner_entry {
    int key;
    leaf_node_t *p_leaf_child;
    inner_node_t *p_inner_child;
}inner_entry_t;

// <STATE>      -2 == FREEZE SLAVE      -1 == FREEZE MASTER     0 == NORMAL        +1 == REGIST MASTER      +2 == REGIST SLVAE
typedef struct inner_node {
    int state;
    int count;
    int level;
    int min_key;
    inner_entry_t entry[MAX_INNER_DEGREE];
    inner_node_t *p_next;
}inner_node_t;

// <STATE>      -2 == FREEZE SLAVE      -1 == FREEZE MASTER     0 == FREE           +1 == REGIST MASTER      +2 == REGIST SLVAE
typedef struct root_node {
    inner_node_t *p_inner;
    hash_entry_t *hash_table; 
} root_node_t;

// <OPERATOR>                       +10 ==: LEAF_SPLIT      -10 ==: LEAF_MERGE

typedef struct smo {
    int operator;
    inner_node_t *p_master_inner, *p_slave_inner;
    leaf_node_t *p_master_header, *p_slave_header;
}smo_t;

////////////////////////////////////////////////////////////////
// INTERFACE
////////////////////////////////////////////////////////////////
root_node_t *rlu_new_root();

void rlu_new_smo(multi_thread_data_t *p_data);

int rlu_lb_tree_add(multi_thread_data_t *p_data, int input, int option);
int rlu_lb_tree_remove(multi_thread_data_t *p_data, int input, int option);

int rlu_lb_tree_search(multi_thread_data_t *p_data, int input, int option);
int rlu_lb_tree_range_scan(multi_thread_data_t *p_data, int start, int end, int option);

int rlu_lb_tree_split(multi_thread_data_t *p_data, int option);
int rlu_lb_tree_merge(multi_thread_data_t *p_data, int option);

int rlu_lb_tree_print(multi_thread_data_t *p_data, int start_lv, int end_lv, int option);


#endif