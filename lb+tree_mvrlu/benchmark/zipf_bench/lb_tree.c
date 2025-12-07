////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "lb_tree.h"
#include "pre_define.h"

#ifdef IS_MVRLU
#include "mvrlu.h"
#else
#include "rlu.h"
#endif
////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// GLOBALS
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// SERVICE_OPERATOR
////////////////////////////////////////////////////////////////
int key_compare(int src, int obj) {
    if (src > obj) {
        return 1;
    } else if (src == obj) {
        return 0;
    } else {
        return 2;
    }
}

////////////////////////////////////////////////////////////////
// PRINT OPERATION
////////////////////////////////////////////////////////////////
// LEAF
void rlu_leaf_print(multi_thread_data_t* p_data, leaf_array_t *p_leaf, int option) {
    printf("----- [%p] LEAF ARRAY META DATA -----\n", p_leaf);
    printf("ARRAY LEVEL         : %d\n", p_leaf->level);
    printf("ARRAY COUNT         : %d\n", p_leaf->count);
    printf("ARRAY KEY           : %d\n", p_leaf->key);
    printf("ARRAY STATE         : %d\n", p_leaf->state);
    printf("ARRAY ENTRY POINTER : %p\n", p_leaf->p_entry);
    printf("ARRAY NEXT POINTER  : %p\n", p_leaf->p_next);

    if (option == 1) {return;}

    printf("----- LEAF NODE DATA -----\n");
    for (int i = 0; i < p_leaf->count; i++) { printf("[%d th KEY %d] : ADDR %p\n", i, p_leaf->p_entry[i].key, &p_leaf->p_entry[i]); }
}

// INNER
void rlu_inner_print(multi_thread_data_t* p_data, inner_array_t *p_inner, int option) {
    printf("----- [%p] INNER ARRAY META DATA -----\n", p_inner);
    printf("ARRAY LEVEL         : %d\n", p_inner->level);
    printf("ARRAY COUNT         : %d\n", p_inner->count);
    printf("ARRAY KEY           : %d\n", p_inner->key);
    printf("ARRAY STATE         : %d\n", p_inner->state);
    printf("ARRAY ENTRY POINTER : %p\n", p_inner->p_entry);
    printf("ARRAY NEXT POINTER  : %p\n", p_inner->p_next);

    if (option == 1) {return;}

    printf("----- INNER NODE DATA -----\n");
    if (p_inner->level > 1) {
        for (int i = 0; i < p_inner->count; i++) { printf("[%d th KEY %d] : ADDR %p\n", i, p_inner->p_entry[i].key, p_inner->p_entry[i].p_inner_child); }
    } else {
        for (int i = 0; i < p_inner->count; i++) { printf("[%d th KEY %d] : ADDR %p\n", i, p_inner->p_entry[i].key, p_inner->p_entry[i].p_leaf_child); }
    }
}

// TREE
int rlu_lb_tree_print(multi_thread_data_t *p_data, int start_lv, int end_lv, int option) {
    rlu_thread_data_t *p_self = p_data->p_rlu_td;
    inner_array_t *p_first_inner, *p_inner;
    leaf_array_t *p_leaf;

    RLU_READER_LOCK(p_self);
    
    p_first_inner = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_root->p_inner));
    p_inner = p_first_inner;

    if (start_lv > p_inner->level) { start_lv = p_inner->level;}
    if (end_lv < 0) { end_lv = start_lv - 1;}
    

    while(p_inner->level >= end_lv) {
        rlu_inner_print(p_data, p_inner, option);
        if (p_inner->p_next) {
            p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->p_next));
        } else {
            if (p_inner->level > 1) {
                p_first_inner = (inner_array_t *)RLU_DEREF(p_self, (p_first_inner->p_entry[0].p_inner_child));
                p_inner = p_first_inner;
            } else {
                p_leaf = (leaf_array_t *) RLU_DEREF(p_self, (p_first_inner->p_entry[0].p_leaf_child));
                break;
            }
        }
    }

    if (end_lv == 1) {
        RLU_READER_UNLOCK(p_self);
        return 1;
    }

    while(p_leaf) {
        rlu_leaf_print(p_data, p_leaf, option);
        p_leaf = (leaf_array_t *)RLU_DEREF(p_self, (p_leaf->p_next));
    }

    RLU_READER_UNLOCK(p_self);
    return 1;
}

// SMO OPERATION
void smo_print(smo_t *p_smo_data, int option) {
    if(!p_smo_data) {
        perror("Invaild parameter for smo_print");
        exit(1);
    }

    switch(p_smo_data->operator) {
        case -10:
            printf("SMO OPERATOR : LEAF MERGE\n");
            break;
        case 10:
            printf("SMO OPERATOR : LEAF SPLIT\n");
            break;
    }
    printf("<INNER NODE>\n");
    // printf("MASTER_PARENT : %p | SLAVE_PARENT : %p | MASTER_CHILD : %p | SLVAE_CHILD : %p\n",
            // p_smo_data->p_master_parent, p_smo_data->p_slave_parent, p_smo_data->p_master_child, p_smo_data->p_slave_child);

    printf("<HEADER NODE>\n");
    // printf("MASTER_HEADER : %p | SLAVE_HEADER : %p\n",
        // p_smo_data->p_master_leaf, p_smo_data->p_slave_leaf);

}

/////////////////////////////////////////////////////////
// INNER ARRAY
/////////////////////////////////////////////////////////
// WRITE
inner_array_t *rlu_new_inner_array() {
    inner_array_t *p_new_inner = (inner_array_t *)RLU_ALLOC(sizeof(inner_array_t));

    p_new_inner->p_entry = (inner_node_t *)calloc(index_meta.inner_node_degree, sizeof(inner_node_t));
    p_new_inner->p_next = NULL;
    
    return p_new_inner;
}

void rlu_free_inner_array(rlu_thread_data_t *p_self, inner_array_t *p_inner) {
    free(p_inner->p_entry);
    RLU_FREE(p_self, p_inner);
}

// READ
int rlu_inner_pos(rlu_thread_data_t *p_self,  inner_array_t **pp_inner, int input, int com) {
    int result, pos, memory;

    inner_array_t *p_inner = *pp_inner;

    while (true) {
        if (input < p_inner->p_next->key) { memory = 2;}
        else {memory = CONTINUE;}

        for (pos = 1; pos < p_inner->count; pos++) {
            result = key_compare(input, p_inner->p_entry[pos].key);

            switch(com) {
                case SEARCH :
                    if (result == 2) { return pos-1;}
                    break;
            }
        }

        if (memory == CONTINUE) { p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->p_next));}
        if (memory == 2) { return pos-1;}
    }
}

inner_array_t *rlu_inner_traversal(multi_thread_data_t *p_data, int input, int level, int option) {
    int curr_level, pos, result;
    root_node_t *p_root;
    inner_array_t *p_inner;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    RLU_READER_LOCK (p_self);

    p_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > level) {
        pos = rlu_inner_pos(p_self, &p_inner, input, SEARCH);
        if (curr_level > 1) {
            p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->p_entry[pos].p_inner_child));
            curr_level = p_inner->level;
        }
    }

    RLU_READER_UNLOCK (p_self);
    return p_inner;
}

/////////////////////////////////////////////////////////
// LEAF ARRAY
/////////////////////////////////////////////////////////
// WRITE
leaf_array_t *rlu_new_leaf_array() {
    leaf_array_t *p_new_leaf = (leaf_array_t *)RLU_ALLOC(sizeof(leaf_array_t));

    p_new_leaf->p_entry = (leaf_node_t *)calloc(index_meta.leaf_node_degree, sizeof(leaf_node_t));
    p_new_leaf->p_next = NULL;

    return p_new_leaf;
}

void rlu_free_leaf_array(rlu_thread_data_t *p_self, leaf_array_t *p_leaf) {
    free(p_leaf->p_entry);
    RLU_FREE(p_self, p_leaf);
}

int rlu_leaf_update(leaf_array_t *p_leaf_array, int input, int pos, int com) {
    if (com == ADD) {
        for (int i = p_leaf_array->count; i > pos; i--) {
            p_leaf_array->p_entry[i].key = p_leaf_array->p_entry[i-1].key;
        }
        p_leaf_array->p_entry[pos].key = input;

        p_leaf_array->count++;
    } else {
        for (int i = pos; i < p_leaf_array->count; i++) {
            p_leaf_array->p_entry[i].key = p_leaf_array->p_entry[i+1].key;
        }

        p_leaf_array->count--;
    }
    return p_leaf_array->count;
}

// READ
int rlu_leaf_pos(rlu_thread_data_t *p_self, leaf_array_t **pp_leaf, int input, int com) {
    int result, pos, memory;

    leaf_array_t *p_leaf = *pp_leaf;

    while (true) {
        if (input < p_leaf->p_next->key) {memory = 2;}
        else {memory = CONTINUE;}

        for (pos = 1; pos <= p_leaf->count; pos++) {
            result = key_compare(input, p_leaf->p_entry[pos-1].key);

            switch(com){
                case SEARCH :
                    if (result == 0) {return pos-1;}
                    if (result == 2) {return FAIL;}
                case SCAN :
                    if (result == 0) {return pos-1;}
                    if (result == 2) {return pos-2;}
                case ADD :
                    if (result == 0) {return FAIL;}
                    if (result == 2) {return pos-1;}
                case REMOVE :
                    if (result == 0) {return pos-1;}
                    if (result == 2) {return FAIL;}
            }
        }

        if (memory == CONTINUE) { p_leaf = (leaf_array_t *)RLU_DEREF(p_self, (p_leaf->p_next));}
        if (memory == 2) { return pos-1;}
    }
}

leaf_array_t *rlu_leaf_traversal(multi_thread_data_t *p_data, int input, int option) {
    int curr_level, pos, result;
    root_node_t *p_root;
    inner_array_t *p_inner;
    leaf_array_t *p_leaf;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    RLU_READER_LOCK (p_self);

    p_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        pos = rlu_inner_pos(p_self, &p_inner, input, SEARCH);

        if (curr_level > 1) {
            p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->p_entry[pos].p_inner_child));
            curr_level = p_inner->level;
        }else {
            p_leaf = (leaf_array_t *)RLU_DEREF(p_self, (p_inner->p_entry[pos].p_leaf_child));
            curr_level = p_leaf->level;
        }

        curr_level--;
    }

    RLU_READER_UNLOCK (p_self);
    return p_leaf;
}

/////////////////////////////////////////////////////////
// TREE     <OPTION> 2 = SPECIFIC PRINT | 1 = NORMAL PRINT | 0 = NOT PRINT
/////////////////////////////////////////////////////////
// WRITE
root_node_t *rlu_new_tree() {
    root_node_t *p_new_root = (root_node_t *)RLU_ALLOC(sizeof(root_node_t));

    leaf_array_t *p_new_leaf_header = rlu_new_leaf_array();
    p_new_leaf_header->state = FREE;
    p_new_leaf_header->count = 0;
    p_new_leaf_header->level = 0;
    p_new_leaf_header->key = HEADER_KEY;
    p_new_leaf_header->p_next = NULL;

    leaf_array_t *p_new_leaf_tail = rlu_new_leaf_array();
    p_new_leaf_tail->state = FREE;
    p_new_leaf_tail->count = 0;
    p_new_leaf_tail->level = 0;
    p_new_leaf_tail->key = TAIL_KEY;
    p_new_leaf_tail->p_next = NULL;
    
    inner_array_t *p_new_inner_header = rlu_new_inner_array();
    p_new_inner_header->state = FREE;
    p_new_inner_header->count = 1;
    p_new_inner_header->level = 1;
    p_new_inner_header->key = HEADER_KEY;
    p_new_inner_header->p_next = NULL;

    inner_array_t *p_new_inner_tail = rlu_new_inner_array();
    p_new_inner_tail->state = FREE;
    p_new_inner_tail->count = 0;
    p_new_inner_tail->level = 1;
    p_new_inner_tail->key = TAIL_KEY;
    p_new_inner_tail->p_next = NULL;

    p_new_leaf_header->p_next = p_new_leaf_tail;
    p_new_inner_header->p_next = p_new_inner_tail;

    p_new_inner_header->p_entry[0].p_leaf_child = p_new_leaf_header;
    p_new_inner_header->p_entry[0].key = p_new_leaf_header->key;

    p_new_root->p_inner = p_new_inner_header;

    rlu_inner_print(NULL, p_new_root->p_inner, 2);
    
    return p_new_root;
}

inner_array_t *rlu_tree_level_down(multi_thread_data_t *p_data, inner_array_t *p_parent, int option) {
    root_node_t *p_root;
    inner_array_t *p_inner, *p_curr, *p_next, *p_child;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_curr = (inner_array_t *)RLU_DEREF(p_self, (p_root->p_inner));
    p_next = (inner_array_t *)RLU_DEREF(p_self, (p_curr->p_next));
    p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_parent));

    if ((p_root->p_inner != p_parent) || (p_inner->count > 1) || (p_next->key != TAIL_KEY)) {
        RLU_READER_UNLOCK(p_self);
        return p_parent;
    }

    if(!RLU_TRY_LOCK(p_self, &p_root) || !RLU_TRY_LOCK_CONST(p_self, p_curr)) {
        RLU_ABORT(p_self);
        goto restart;
    }

    p_child = (inner_array_t *)RLU_DEREF(p_self, (p_curr->p_entry[0].p_inner_child));

    RLU_ASSIGN_PTR(p_self, &(p_root->p_inner), p_child);

    rlu_free_inner_array(p_self, p_curr);
    rlu_free_inner_array(p_self, p_next);

    RLU_READER_UNLOCK(p_self);
    return p_root->p_inner;
}

void rlu_tree_level_up(multi_thread_data_t *p_data, int parent_level, int option) {
    root_node_t *p_root;
    inner_array_t *p_inner, *p_header, *p_tail;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_root->p_inner));

    if(!RLU_TRY_LOCK(p_self, &p_root) || !RLU_TRY_LOCK(p_self, &p_inner)) {
        RLU_ABORT(p_self);
        goto restart;
    }

    p_tail = rlu_new_inner_array();
    p_tail->state = FREE;
    p_tail->count = 0;
    p_tail->level = parent_level;
    p_tail->key = TAIL_KEY;
    p_tail->p_next = NULL;

    p_header = rlu_new_inner_array();
    p_header->state = FREE;
    p_header->count = 0;
    p_header->level = parent_level;
    p_header->key = HEADER_KEY;
    p_header->p_entry[0].key = p_inner->key;

    p_header->p_next = p_tail;

    RLU_ASSIGN_PTR(p_self, &(p_header->p_entry[0].p_inner_child), p_inner);
    RLU_ASSIGN_PTR(p_self, &(p_root->p_inner), p_header);

    RLU_READER_UNLOCK(p_self);
    return;
}

// OPERATOR
int rlu_leaf_search(multi_thread_data_t *p_data, leaf_array_t *p_origin, int input, int option) {
    if (!p_data || !p_origin) {return LOST;}
    
    int result, pos;
    leaf_array_t *p_curr;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    RLU_READER_LOCK(p_self);

    p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_origin));

    pos = rlu_leaf_pos(p_self, &p_curr, input, SEARCH);

    RLU_READER_UNLOCK(p_self);

    if (pos == FAIL) { return FAIL;}
    else {return SUCCESS;}
}

int rlu_leaf_range_scan(multi_thread_data_t *p_data, leaf_array_t *p_origin, int input, int scan_num, int option) {
    if(!p_data || !p_origin) {return LOST;}

    int result, pos, count;
    leaf_array_t *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    result = 0;
    count = 0;

    RLU_READER_LOCK(p_self);

    p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_origin));

    pos = rlu_leaf_pos(p_self, &p_curr, input, SCAN);

    p_next = (leaf_array_t *)RLU_DEREF(p_self, (p_curr->p_next));

    while(p_next) {
        for (pos; pos < p_curr->count; pos++) {
            result = p_curr->p_entry[pos].key;
            count++;

            if (count >= scan_num) { break;}
        }

        p_curr = p_next;
        p_next = (leaf_array_t *)RLU_DEREF(p_self, (p_curr->p_next));
        pos = 0;
    }

    RLU_READER_UNLOCK(p_self);

    if (count > 0) { return SUCCESS;}
    else {return FAIL;}
}

int rlu_leaf_add(multi_thread_data_t *p_data, leaf_array_t *p_origin, int input, int option) {
    if (!p_data || !p_origin) {return LOST;}
    
    int result, pos, split_thres;
    leaf_array_t *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    split_thres = (index_meta.leaf_node_degree * index_meta.split_thres_r) / 100;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_origin));

    pos = rlu_leaf_pos(p_self, &p_curr, input, ADD);

    p_next = (leaf_array_t *)RLU_DEREF(p_self, (p_curr->p_next));

    if (result > FAIL) {
        if (!RLU_TRY_LOCK(p_self, &p_curr)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        rlu_leaf_update(p_curr, input, pos, ADD);

        if (p_curr->count > split_thres && p_curr->state == FREE) {
            p_curr->state = SPLIT_REGIST;
            RLU_ASSIGN_PTR(p_self, &(p_data->p_smo->p_master_leaf), p_curr);
            p_data->p_smo->operator = 10;
        }
 
        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return FAIL;
    }
}

int rlu_leaf_remove(multi_thread_data_t *p_data, leaf_array_t *p_origin, int input, int option) {
    if (!p_data || !p_origin) {return LOST;}
    
    int result, pos, merge_thres;
    leaf_array_t *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    merge_thres = (index_meta.leaf_node_degree * index_meta.merge_thres_r) / 100;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_origin));

    pos = rlu_leaf_pos(p_self, &p_curr, input, REMOVE);

    p_next = (leaf_array_t *)RLU_DEREF(p_self, (p_curr->p_next));

    if (result > FAIL) {
        if (!RLU_TRY_LOCK(p_self, &p_curr)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        rlu_leaf_update(p_curr, input, pos, REMOVE);

        if (p_curr->count < merge_thres && p_curr->state == FREE) {
            p_curr->state = MERGE_REGIST;
            RLU_ASSIGN_PTR(p_self, &(p_data->p_smo->p_master_leaf), p_curr);
            p_data->p_smo->operator = -10;
        }

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return FAIL;
    }
}


int rlu_lb_tree_search(multi_thread_data_t *p_data, int input, int option) {
    leaf_array_t *p_leaf;

    p_leaf = rlu_leaf_traversal(p_data, input, option);

    return rlu_leaf_search(p_data, p_leaf, input, option);
}

int rlu_lb_tree_range_scan(multi_thread_data_t *p_data, int start, int scan_num, int option) {
    leaf_array_t *p_leaf;

    p_leaf = rlu_leaf_traversal(p_data, start, option);

    return rlu_leaf_range_scan(p_data, p_leaf, start, scan_num, option);
}

int rlu_lb_tree_add(multi_thread_data_t *p_data, int input, int option) {
    leaf_array_t *p_leaf;

    p_leaf = rlu_leaf_traversal(p_data, input, option);

    return rlu_leaf_add(p_data, p_leaf, input, option);

    return 1;
}

int rlu_lb_tree_remove(multi_thread_data_t *p_data, int input, int option) {
    leaf_array_t *p_leaf;

    p_leaf = rlu_leaf_traversal(p_data, input, option);

    return rlu_leaf_remove(p_data, p_leaf, input, option);
}

////////////////////////////////////////////////////////////////
// SPLIT MERGE OPERATOR
////////////////////////////////////////////////////////////////
// NEW      
void rlu_new_smo(multi_thread_data_t *p_data) {
    p_data->p_smo = (smo_t *)calloc(1, sizeof(smo_t));

    return;
}

/////////////////////////////////////////////////////////
// LEAF ARRAY
/////////////////////////////////////////////////////////
// WRITE
void rlu_leaf_array_split(multi_thread_data_t *p_data, leaf_array_t *p_master, leaf_array_t *p_slave, int option) {
    for (int i = 0; i < p_slave->count; i++) {
        p_slave->p_entry[i] = p_master->p_entry[p_master->count + i];
        memset(&p_master->p_entry[p_master->count + i], 0, sizeof(leaf_node_t));
    }
}

int rlu_leaf_release(multi_thread_data_t *p_data, int command, int option) {    
    int operator;
    leaf_array_t *p_master, *p_slave;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    operator = p_data->p_smo->operator;
    RLU_READER_LOCK(p_self);

    if (command == 1) {
        p_master = (leaf_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_leaf));
        if(!RLU_TRY_LOCK(p_self, &p_master)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_master->state == MERGE_REGIST) || (operator > 0 && p_master->state == SPLIT_REGIST)) {
            p_master->state = FREE;
            RLU_READER_UNLOCK(p_self);
            return SUCCESS;
        }
    } else if (command == 2) {
        p_slave = (leaf_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_leaf));
        if(!RLU_TRY_LOCK(p_self, &p_slave)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_slave->state == MERGE_REGIST) || (operator > 0 && p_slave->state == SPLIT_REGIST)) {
            p_slave->state = FREE;
            RLU_READER_UNLOCK(p_self);
            return SUCCESS;
        }

    } else if (command == 3) {
        p_master = (leaf_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_leaf));
        p_slave = (leaf_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_leaf));
        if (!RLU_TRY_LOCK(p_self, &p_master) || !RLU_TRY_LOCK(p_self, &p_slave)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_master->state == MERGE_REGIST && p_slave->state == MERGE_REGIST) ||
            (operator > 0 && p_master->state == SPLIT_REGIST && p_slave->state == SPLIT_REGIST)) {
                p_master->state = FREE;
                p_slave->state = FREE;
                RLU_READER_UNLOCK(p_self);
                return SUCCESS;
            }
    }
    return NOTHING;
}

int rlu_leaf_split(multi_thread_data_t *p_data, int option) {
    int i, split_point;
    leaf_array_t *p_curr, *p_next, *p_new;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_leaf));
    p_next = (leaf_array_t *)RLU_DEREF(p_self, (p_curr->p_next));

    split_point = (p_curr->count * index_meta.distri_r) / 100;
    
    if (!RLU_TRY_LOCK(p_self, &p_curr) || !RLU_TRY_LOCK(p_self, &p_next)) {
        RLU_ABORT(p_self);
        goto restart;
    }
    
    p_new = rlu_new_leaf_array();
    p_new->state = SPLIT_REGIST;
    p_new->count = split_point;
    p_new->level = p_curr->level;

    p_curr->count = p_curr->count - split_point;

    rlu_leaf_array_split(p_data, p_curr, p_new, option);

    p_new->key = p_new->p_entry[0].key;

    RLU_ASSIGN_PTR(p_self, &(p_new->p_next), p_next);
    RLU_ASSIGN_PTR(p_self, &(p_curr->p_next), p_new);

    p_data->p_smo->p_slave_leaf = p_new;

    RLU_READER_UNLOCK(p_self);
    return SUCCESS;
}

int rlu_header_partner_search(multi_thread_data_t *p_data, inner_array_t **pp_parent, int parent_level, int option) {

    int i, master_key, curr_level, pos, master_number, slave_number, result;

    root_node_t *p_curr_root;
    inner_array_t *p_inner;
    leaf_array_t *p_regist_header;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    master_key = p_data->p_smo->p_master_leaf->key;

restart:
    pos = 0;
    result = 0;
    p_regist_header = NULL;
    
    RLU_READER_LOCK(p_self);
    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        pos = rlu_inner_pos(p_self, &p_inner, master_key, 10);
        if (curr_level > 1) {
            p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->p_entry[pos].p_inner_child));
            curr_level = p_inner->level;
        }else {curr_level--;}
    }

    if (p_curr_root->p_inner->level == 1 && p_inner->count == 1) {
        RLU_READER_UNLOCK(p_self);
        return FAIL;
    }

    if (pos < p_inner->count - 1) {
        p_regist_header = (leaf_array_t *)RLU_DEREF(p_self, (p_inner->p_entry[pos + 1].p_leaf_child));
        if (p_regist_header->state == FREE) {result = 1;}
    }

    if (!p_regist_header && pos > 0) {
        p_regist_header = (leaf_array_t *)RLU_DEREF(p_self, p_inner->p_entry[pos - 1].p_leaf_child);
        if (p_regist_header->state == FREE) {result = -1;}
    }

    if (result) {
        if(!RLU_TRY_LOCK(p_self, &p_regist_header) || !RLU_TRY_LOCK(p_self, &p_inner)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_regist_header->state = MERGE_REGIST;

        if (result == 1) {
            master_number = pos;
            slave_number = pos + result;
        } else {
            master_number = pos + result;
            slave_number = pos;
        }
        
        p_data->p_smo->p_master_leaf = p_inner->p_entry[master_number].p_leaf_child;
        p_data->p_smo->p_slave_leaf = p_inner->p_entry[slave_number].p_leaf_child;

        for (i = slave_number; i < p_inner->count; i++) {p_inner->p_entry[i] = p_inner->p_entry[i + 1];}

        p_inner->count--;

        RLU_ASSIGN_PTR(p_self, pp_parent, p_inner);

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_ABORT(p_self);
        if(pos == 0 && p_inner->count == 1) {return FAIL;}
        else {goto restart;}
    }
}

int rlu_header_merge(multi_thread_data_t *p_data, int option) {
    if(!p_data) {
        perror("Invalid parameter for rlu_header_merge");
        exit(1);
    }
    
    int result, slave_key;
    leaf_array_t *p_header, *p_prev, *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    slave_key = p_data->p_smo->p_slave_leaf->key;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_prev = (leaf_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_leaf));
    p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_prev->p_next));
    p_header = p_prev;

    while(1) {
        result = key_compare(slave_key, p_curr->key);

        if (result == 2) {
            result = NOTHING;
            break;
        }
        
        if (result == 0) { 
            result = SUCCESS;
            break;
        }
        
        p_prev = p_curr;
        p_curr = (leaf_array_t *)RLU_DEREF(p_self, (p_prev->p_next));
        
    }

    if (result) {
        p_next = (leaf_array_t *)RLU_DEREF(p_self, (p_curr->p_next));

        if (!RLU_TRY_LOCK(p_self, &p_header) || !RLU_TRY_LOCK(p_self, &p_prev) || !RLU_TRY_LOCK_CONST(p_self, p_curr)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_header->count = p_header->count + p_curr->count;

        RLU_ASSIGN_PTR (p_self, &(p_prev->p_next), p_next);

        rlu_free_leaf_array(p_self, p_curr);

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

}

/////////////////////////////////////////////////////////
// INNER NODE 
/////////////////////////////////////////////////////////
// WRITE
int rlu_inner_release(multi_thread_data_t *p_data, int command, int option) {
    int operator, result;
    inner_array_t *p_master, *p_slave;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    operator = p_data->p_smo->operator;
    RLU_READER_LOCK(p_self);

    if (command == 1) {
        p_master = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
        if(!RLU_TRY_LOCK(p_self, &p_master)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_master->state == MERGE_REGIST) || (operator > 0 && p_master->state == SPLIT_REGIST)) {
            p_master->state = FREE;
            result = SUCCESS;
        }
    } else if (command == 2) {
        p_slave = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_inner));
        if(!RLU_TRY_LOCK(p_self, &p_slave)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_slave->state == MERGE_REGIST) || (operator > 0 && p_slave->state == SPLIT_REGIST)) {
            p_slave->state = FREE;
            result = SUCCESS;
        }
    } else {
        p_master = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
        p_slave = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_inner));
        if (!RLU_TRY_LOCK(p_self, &p_master) || !RLU_TRY_LOCK(p_self, &p_slave)) {
            RLU_ABORT(p_self);
            goto restart;
        }
        if ((operator < 0 && p_master->state == MERGE_REGIST && p_slave->state == MERGE_REGIST) ||
            (operator > 0 && p_master->state == SPLIT_REGIST && p_slave->state == SPLIT_REGIST)) {
                p_master->state = FREE;
                p_slave->state = FREE;
                result = SUCCESS;
            }
    }

    if (result) {
        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    }

    return NOTHING;
}

int rlu_inner_smo_check(multi_thread_data_t *p_data, inner_array_t *p_master, int command) {
        if(!p_data || !p_master) {return LOST;}

        int split_thres, merge_thres;
        inner_array_t *p_inner;
        rlu_thread_data_t *p_self = p_data->p_rlu_td;

        split_thres = (index_meta.inner_node_degree * index_meta.split_thres_r) / 100;
        merge_thres = (index_meta.inner_node_degree * index_meta.merge_thres_r) / 100;

restart:
        RLU_READER_LOCK(p_self);

        p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_master));

        if (command == 1 && p_inner->count > split_thres && p_inner->state == FREE) {
            if (!RLU_TRY_LOCK(p_self, &p_inner)) {
                RLU_ABORT(p_self);
                goto restart;
            }

            p_inner->state = SPLIT_REGIST;
            p_data->p_smo->operator = 10 + p_inner->level;
            RLU_ASSIGN_PTR(p_self, &(p_data->p_smo->p_master_inner), p_inner);
            RLU_READER_UNLOCK(p_self);
            return SUCCESS; 
        }

        if(command == -1 && p_inner->level < p_data->p_root->p_inner->level && p_inner->count < merge_thres && p_inner->state == FREE) {
            if (!RLU_TRY_LOCK(p_self, &p_inner)) {
                RLU_ABORT(p_self);
                goto restart;
            }

            p_inner->state = MERGE_REGIST;
            p_data->p_smo->operator = -10 - p_inner->level;
            RLU_ASSIGN_PTR(p_self, &(p_data->p_smo->p_master_inner), p_inner);
            RLU_READER_UNLOCK(p_self);
            return SUCCESS;
        }

        RLU_READER_UNLOCK(p_self);
        return NOTHING;
    }   

int rlu_inner_array_split(multi_thread_data_t *p_data, inner_array_t *p_master, inner_array_t *p_slave, int option) {
    for (int i = 0; i < p_slave->count; i++) {
        p_slave->p_entry[i] = p_master->p_entry[p_master->count + i];
        memset(&p_master->p_entry[p_master->count + i], 0, sizeof(inner_node_t));
    }
}

int rlu_inner_split(multi_thread_data_t *p_data, int option) {
    int i, split_point;
    inner_array_t *p_curr, *p_next, *p_new;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_curr = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
    p_next = (inner_array_t *)RLU_DEREF(p_self, (p_curr->p_next));

    split_point = (p_curr->count * index_meta.distri_r) / 100;

    if (!RLU_TRY_LOCK(p_self, &p_curr) || !RLU_TRY_LOCK(p_self, &p_next)) {
        RLU_ABORT(p_self);
        goto restart;
    }

    p_new = rlu_new_inner_array();

    p_new->state = SPLIT_REGIST;
    p_new->count = split_point;
    p_new->level = p_curr->level;

    p_curr->count = p_curr->count - split_point;

    rlu_inner_array_split(p_data, p_curr, p_new, option);

    p_new->key = p_new->p_entry[0].key;

    RLU_ASSIGN_PTR(p_self, &(p_new->p_next), p_next);
    RLU_ASSIGN_PTR(p_self, &(p_curr->p_next), p_new);

    p_data->p_smo->p_slave_inner = p_new;

    RLU_READER_UNLOCK(p_self);
    return  SUCCESS;
}

int rlu_inner_split_update(multi_thread_data_t *p_data, inner_array_t *p_inner, int option) {
    int i, m_pos, child_key, result;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    result = 0;

    RLU_READER_LOCK(p_self);

    p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner));
    

    if (p_inner->level == 1) { child_key = p_data->p_smo->p_master_leaf->key;}
    else { child_key = p_data->p_smo->p_master_inner->key;}

    for (m_pos = 0; m_pos < p_inner->count; m_pos++) {
        if (p_inner->p_entry[m_pos].key == child_key) {
            result = 1;
            break;
        }
    }

    if (result) {
        if (!RLU_TRY_LOCK(p_self, &p_inner)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        for (i = p_inner->count - 1; i >= m_pos + 1; i--) {
            p_inner->p_entry[i + 1] = p_inner->p_entry[i];
        }

        if (p_inner->level == 1) {
            p_inner->p_entry[m_pos + 1].key = p_data->p_smo->p_slave_leaf->key;
            p_inner->p_entry[m_pos + 1].p_leaf_child = p_data->p_smo->p_slave_leaf;
        } else {
            p_inner->p_entry[m_pos + 1].p_inner_child = p_data->p_smo->p_slave_inner;
            p_inner->p_entry[m_pos + 1].key = p_data->p_smo->p_slave_inner->key;
        }

        p_inner->count++;

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

}

int rlu_parent_split_update(multi_thread_data_t *p_data, int parent_level, int option) {
    int curr_level, master_key, pos, result;
    root_node_t *p_root;
    inner_array_t *p_inner;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    if (parent_level == 1) {master_key = p_data->p_smo->p_master_leaf->key;}
    else {master_key = p_data->p_smo->p_master_inner->key;}

    curr_level = p_data->p_root->p_inner->level;

restart:
    result = 0;

    if (curr_level < parent_level) { rlu_tree_level_up(p_data, parent_level, option);}

    p_inner = rlu_inner_traversal(p_data, master_key, parent_level, option);

    result = rlu_inner_split_update(p_data, p_inner, option);

    if(result != SUCCESS) {goto restart;}

    if (parent_level > 1) {result = rlu_inner_release(p_data, 3, option);}
    else {result = rlu_leaf_release(p_data, 3, option);}

    if (result == SUCCESS) {rlu_inner_smo_check(p_data, p_inner, 1);}

    return result;

}

int rlu_inner_partner_search(multi_thread_data_t *p_data, inner_array_t **pp_parent, int parent_level, int option) {
//     int i, master_key, curr_level, pos, master_number, slave_number, result;

//     root_node_t *p_curr_root;
//     inner_array_t *p_inner, *p_regist_inner;
//     rlu_thread_data_t *p_self = p_data->p_rlu_td;

//     master_key = p_data->p_smo->p_master_inner->key;

// restart:
//     result = 0;
//     pos = 0;
//     p_regist_inner = NULL;
    
//     RLU_READER_LOCK(p_self);
//     p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
//     p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
//     curr_level = p_inner->level;

//     while (curr_level >= parent_level) {
//         pos = rlu_inner_entry_search(p_data, &p_inner, master_key, option);
//         if(curr_level > parent_level) {
//             p_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->entry[pos].p_inner_child));
//             curr_level = p_inner->level;
//         } else {break;}
//     }


//     if (pos < p_inner->count - 1) {
//         p_regist_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->entry[pos + 1].p_inner_child));
//         if (p_regist_inner->state == FREE) {result = 1;}
//     }

//     if (!p_regist_inner && pos > 0) {
//         p_regist_inner = (inner_array_t *)RLU_DEREF(p_self, (p_inner->entry[pos - 1].p_inner_child));
//         if (p_regist_inner->state == FREE) {result = -1;}
//     }

//     if (result) {
//         if(!RLU_TRY_LOCK(p_self, &p_regist_inner) || !RLU_TRY_LOCK(p_self, &p_inner)) {
//             RLU_ABORT(p_self);
//             goto restart;
//         }

//         p_regist_inner->state = MERGE_REGIST;

//         if (result == 1) {
//             master_number = pos;
//             slave_number = pos + result;
//         } else {
//             master_number = pos + result;
//             slave_number = pos;
//         }

//         p_data->p_smo->p_master_inner = p_inner->entry[master_number].p_inner_child;
//         p_data->p_smo->p_slave_inner = p_inner->entry[slave_number].p_inner_child;

//         for (i = slave_number; i < p_inner->count; i++) {p_inner->entry[i] = p_inner->entry[i + 1];}

//         p_inner->count--;

//         RLU_ASSIGN_PTR(p_self, pp_parent, p_inner);

//         RLU_READER_UNLOCK(p_self);
//         return SUCCESS;
//     } else {
//         RLU_ABORT(p_self);
//         goto restart;
//     }

}

int rlu_inner_merge(multi_thread_data_t *p_data, int option) {
    //  int i;
    //  inner_array_t *p_prev, *p_curr, *p_next;
    //  rlu_thread_data_t *p_self = p_data->p_rlu_td;
// 
// restart:
    // RLU_READER_LOCK(p_self);
// 
    // p_prev = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
    // p_curr = (inner_array_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_inner));
    // 
    // if (!RLU_TRY_LOCK(p_self, &p_prev) || !RLU_TRY_LOCK_CONST(p_self, p_curr)) {
    //    RLU_ABORT(p_self);
    //    goto restart;
    // }
// 
    // p_next = (inner_array_t *)RLU_DEREF(p_self, (p_curr->p_next));
// 
    // for(i = 0; i < p_curr->count; i++) {p_prev->entry[p_prev->count + i] = p_curr->entry[i];}
// 
    // p_prev->count = p_prev->count + p_curr->count;
// 
    // RLU_ASSIGN_PTR(p_self, &(p_prev->p_next), p_next);
// 
    // rlu_free_inner_array(p_self, p_curr);
// 
    // RLU_READER_UNLOCK(p_self);
// 
    // return SUCCESS;
}

/////////////////////////////////////////////////////////
// TREE
/////////////////////////////////////////////////////////
// SPLIT
int rlu_lb_tree_split(multi_thread_data_t *p_data, int option) {
    int result, operator, child_level, parent_level;

    operator = p_data->p_smo->operator;
    child_level = operator % 10;
    parent_level = child_level + 1;

    result = 0;

    printf("operator %d\n", operator);

    if (child_level == 0) {result = rlu_leaf_split(p_data, option);}
    else {result = rlu_inner_split(p_data, option);}

    printf("split done\n");

    if(result) {result = rlu_parent_split_update(p_data, parent_level, option);}

    return result;
}

// MERGE
int rlu_lb_tree_merge(multi_thread_data_t *p_data, int option) {
    // int result, operator, child_level, parent_level;
    // inner_array_t *p_parent_inner;

    // operator = p_data->p_smo->operator;
    // child_level = abs(operator % 10);
    // parent_level = child_level + 1;

    // result = 0;

    // if (child_level == 0) {
    //     result = rlu_header_partner_search(p_data, &p_parent_inner, parent_level, option);

    //     if (result == SUCCESS) {result = rlu_header_merge(p_data, option);}
    //     else if (result == FAIL) {result = NOTHING;}
    //     else if (result == LOST) {perror("LOST HEADER IN THE LB TREE MERGE"); exit(1);}

    //     rlu_leaf_release(p_data, 1, option);

    // } else {
    //     result = rlu_inner_partner_search(p_data, &p_parent_inner, parent_level, option);

    //     if (result == SUCCESS) {result = rlu_inner_merge(p_data, option);}
    //     else if (result == FAIL) {result = NOTHING;}
    //     else if (result == LOST) {perror("LOST MASTSER IN THE LB TREE MERGE"); exit(1);}

    //     rlu_inner_release(p_data, 1, option);
    // }

    // if (result == SUCCESS) {
    //     p_parent_inner = rlu_tree_level_down(p_data, p_parent_inner, option);
    //     rlu_inner_smo_check(p_data, p_parent_inner, -1);
    // }

    // return result;
}


