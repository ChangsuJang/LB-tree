////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "lb_tree.h"

////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// GLOBALS
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// SERVICE_OPERATOR
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// LEAF NODE
/////////////////////////////////////////////////////////
// NEW
leaf_node_t *pure_new_leaf() {
    return NULL;
}

leaf_node_t *rlu_new_leaf() {
    leaf_node_t *p_new_leaf;
    p_new_leaf = (leaf_node_t *)RLU_ALLOC(sizeof(leaf_node_t));

    if (!p_new_leaf) {
        printf("Failed to allocate rlu_new_leaf");
        exit(1);
    }
    p_new_leaf->type = 0;
    p_new_leaf->state = 0;
    p_new_leaf->count = 0;
    p_new_leaf->p_next = NULL;

    return p_new_leaf;
}


void rlu_free_leaf(rlu_thread_data_t *p_self, leaf_node_t *p_leaf) {
    if (!p_self) {
        perror("Invaild parameter for rlu_free_leaf");
        exit(1);
    } else {RLU_FREE(p_self, p_leaf);}
}

int pure_leaf_search(single_thread_data_t *p_data, leaf_node_t *p_header, item_t input, int option) {
    return 1;
}

int rlu_leaf_search(rlu_multi_thread_data_t *p_data, leaf_node_t *p_header, item_t input, int option) {
    if (!p_data || !p_header) {return LOST;}
    
    int result;
    leaf_node_t *p_prev, *p_curr;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    result = 0;
    RLU_READER_LOCK (p_self);

    p_prev = (leaf_node_t *)RLU_DEREF(p_self, (p_header));
    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));

    if(!p_prev) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

    while(1) {
        result = key_compare(input.key, p_curr->item.key);

        if (result == 2) {
            result = NOTHING;
            break;
        }
        
        if (result == 0 && p_curr->type == 0) { 
            result = SUCCESS;
            break;
        }
        
        p_prev = p_curr;
        p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));
    }

    RLU_READER_UNLOCK (p_self);

    return result;
}

int pure_leaf_range_scan(single_thread_data_t *p_data, leaf_node_t *p_header, int start, int end, int option) {
    return 1;
}

int rlu_leaf_range_scan(rlu_multi_thread_data_t *p_data, leaf_node_t *p_header, int start, int end, int option) {
    if(!p_data || !p_header) {return LOST;}

    int result;
    leaf_node_t *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    result = 0;
    RLU_READER_LOCK (p_self);

    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_header));
    p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));

    if (!p_curr) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

    while(1) {
        result = key_compare(start, p_next->item.key);

        if (result == 2) {break;}

        p_curr = p_next;
        p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
    }

    while(1) {
        result = key_compare(end, p_next->item.key);
        if (result == 2) {break;}
    
        p_curr = p_next;
        p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
        p_data->nb_scan++;
    }

    RLU_READER_UNLOCK(p_self);
    return SUCCESS;
}

// WRITE
int pure_leaf_add(single_thread_data_t *p_data, leaf_node_t *p_header, item_t input, int option) {
    return 1;
}

int rlu_leaf_add(rlu_multi_thread_data_t *p_data, leaf_node_t *p_header, item_t input, int option) {
    if (!p_data || !p_header) {return LOST;}
    
    int result, split_thres;
    leaf_node_t *p_curr, *p_next, *p_new;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    split_thres = (lb_meta.leaf_node_degree * lb_meta.split_thres_r) / 100;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_header));
    p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
    p_header = p_curr;

    if (!p_header) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }
    
    while(1) {
        result = key_compare(input.key, p_next->item.key);

        if (result == 0 && p_next->type == 0) {break;}
        
        if (result == 2) {break;}

        if (p_next->type == 1) {p_header = p_next;}

        p_curr = p_next;
        p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
    }

    if (result) {
        if (!RLU_TRY_LOCK(p_self, &p_header) || !RLU_TRY_LOCK(p_self, &p_curr) || !RLU_TRY_LOCK(p_self, &p_next)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_new = rlu_new_leaf();
        p_new->type = 0;
        p_new->item = input;

        RLU_ASSIGN_PTR(p_self, &(p_new->p_next), p_next);
        RLU_ASSIGN_PTR(p_self, &(p_curr->p_next), p_new);

        p_header->count++;

        if (p_header->count > split_thres && p_header->state == FREE) {
            p_header->state = SPLIT_REGIST;
            RLU_ASSIGN_PTR(p_self, &(p_data->p_smo->p_master_header), p_header);
            p_data->p_smo->operator = 10;
        }

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return NOTHING;
    }
}

int pure_leaf_remove(single_thread_data_t *p_data, leaf_node_t *p_header, item_t input, int option) {
    return 1;
}

int rlu_leaf_remove(rlu_multi_thread_data_t *p_data, leaf_node_t *p_header, item_t input, int option) {
    if (!p_data || !p_header) {return LOST;}
    
    int result, merge_thres;
    leaf_node_t *p_prev, *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    merge_thres = (lb_meta.leaf_node_degree * lb_meta.merge_thres_r) / 100;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_prev = (leaf_node_t *)RLU_DEREF(p_self, (p_header));
    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));
    p_header = p_prev;

    if (!p_header) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

    while(1) {
        result = key_compare(input.key, p_curr->item.key);

        if (result == 2) {
            result = 0;
            break;
        }
        
        if (result == 0 && p_curr->type == 0) { 
            result = 1;
            break;
        }

        if (p_curr->type == 1) {p_header = p_curr;}
        
        p_prev = p_curr;
        p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));
    }

    if (result) {
        p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));

        if (!RLU_TRY_LOCK(p_self, &p_header) || !RLU_TRY_LOCK(p_self, &p_prev) || !RLU_TRY_LOCK(p_self, &p_curr)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        RLU_ASSIGN_PTR (p_self, &(p_prev->p_next), p_next);

        rlu_free_leaf(p_self, p_curr);

        p_header->count--;

        if (p_header->count < merge_thres && p_header->state == FREE) {
            p_header->state = MERGE_REGIST;
            RLU_ASSIGN_PTR(p_self, &(p_data->p_smo->p_master_header), p_header);
            p_data->p_smo->operator = -10;
        }

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return NOTHING;
    }
}

int pure_leaf_update(single_thread_data_t *p_data, leaf_node_t *p_header, item_t input, item_t update_item, int option) {
    return 1;
}

int rlu_leaf_update(rlu_multi_thread_data_t *p_data, leaf_node_t *p_header, item_t input, item_t update_item, int option) {
    if (!p_data || !p_header) {return LOST;}
    
    int result;
    leaf_node_t *p_prev, *p_curr;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_prev = (leaf_node_t *)RLU_DEREF(p_self, (p_header));
    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));

    if (!p_prev) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

    while(1) {
        result = key_compare(input.key, p_curr->item.key);

        if (result == 2) {
            result = 0;
            break;
        }
        
        if (result == 0 && p_curr->type == 0) { 
            result = 1;
            break;
        }
        
        p_prev = p_curr;
        p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));
    }

    if (result) {
        if (!RLU_TRY_LOCK(p_self, &p_curr)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_curr->item.value = update_item.value;

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return NOTHING;
    }
}

/////////////////////////////////////////////////////////
// INNER NODE
/////////////////////////////////////////////////////////
// NEW
inner_node_t *pure_new_inner() {
    return NULL;
}

inner_node_t *rlu_new_inner() {
    inner_node_t *p_new_inner = (inner_node_t *)RLU_ALLOC(sizeof(inner_node_t));
    if(!p_new_inner) {
        perror("Failed to allocate rlu inner node");
        exit(1);
    }

    memset(p_new_inner->entry, 0 ,MAX_INNER_DEGREE * sizeof(inner_entry_t));
    
    return p_new_inner;
}

// READ
int pure_inner_entry_search(single_thread_data_t *p_data, inner_node_t **pp_inner, int input_key, int option) {
    return 1;
}

int rlu_inner_entry_search(rlu_multi_thread_data_t *p_data, inner_node_t **pp_inner, int input_key, int option) {
    if(!p_data || !pp_inner) {
        perror("Invalid parameter for rlu_header_entry_search");
        exit(1);
    }

    rlu_thread_data_t *p_self = p_data->p_rlu_td;
    int i, result;
    inner_node_t *p_inner = *pp_inner;

    result = 0;

    while (1) {
        result = key_compare(input_key, p_inner->p_next->min_key);
        if (result == 2) {
            for (i = 1; i < p_inner->count; i++) {
                if (input_key < p_inner->entry[i].key) {
                    *pp_inner = p_inner;
                    return i-1;
                }
            }
            *pp_inner = p_inner;
            return i-1;
        } else {p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->p_next));}
    }
}

// FREE
void pure_free_inner(inner_node_t *p_inner) {
    return;
}

void rlu_free_inner(rlu_thread_data_t *p_self ,inner_node_t *p_inner) {
    if (!p_self || !p_inner) {
        perror("Invalid parameter for rlu_free_inner");
        exit(1);
    } else {RLU_FREE(p_self, p_inner);}
}

/////////////////////////////////////////////////////////
// ROOT NODE
/////////////////////////////////////////////////////////
// NEW
root_node_t *pure_new_root() {
    return NULL;
}

root_node_t *rlu_new_root() {
    root_node_t *p_new_root = (root_node_t *)RLU_ALLOC(sizeof(root_node_t));
    if (!p_new_root) {
        perror("Failed to allocate rlu root node");
        exit(1);
    }
    
    p_new_root->hash_table = (hash_entry_t *)RLU_ALLOC(HASH_TABLE_SIZE * sizeof(hash_entry_t));
    if (!p_new_root->hash_table) {
        perror("Failed to allocate hash table of root node");
        exit(1);
    }


    leaf_node_t *p_new_head = rlu_new_leaf();
    p_new_head->state = FREE;
    p_new_head->count = 0;
    p_new_head->type = HEADER_TYPE;
    p_new_head->item.key = ITEM_MIN_KEY - 1;

    leaf_node_t *p_new_tail = rlu_new_leaf();
    p_new_tail->type = TAIL_TYPE;
    p_new_tail->item.key = ITEM_MAX_KEY + 1;
    p_new_tail->p_next = NULL;

    p_new_head->p_next = p_new_tail;

    inner_node_t *p_new_inner = rlu_new_inner();
    p_new_inner->state = FREE;
    p_new_inner->count = 1;
    p_new_inner->level = 1;
    p_new_inner->entry[0].key = p_new_head->item.key;
    p_new_inner->min_key = p_new_inner->entry[0].key;
    p_new_inner->entry[0].p_leaf_child = p_new_head;

    inner_node_t *p_new_inner_tail = rlu_new_inner();
    p_new_inner_tail->state = FREE;
    p_new_inner_tail->count = 0;
    p_new_inner_tail->level = 1;
    p_new_inner_tail->min_key = ITEM_MAX_KEY + 1;
    p_new_inner_tail->p_next = NULL;

    p_new_inner->p_next = p_new_inner_tail;
    p_new_root->p_inner = p_new_inner;

    return p_new_root;
}

// WRITE
inner_node_t *pure_root_level_down(single_thread_data_t *p_data, inner_node_t **pp_parent, int option) {
    return NULL;
}

inner_node_t *rlu_root_level_down(rlu_multi_thread_data_t *p_data, inner_node_t *p_parent, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_root_level_down");
        exit(1);
    }

    root_node_t *p_curr_root;
    inner_node_t *p_inner, *p_curr, *p_next, *p_child;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_curr = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    p_next = (inner_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_parent));

    if ((p_curr_root->p_inner != p_parent) || (p_inner->count > 1) || (p_next->min_key != ITEM_MAX_KEY + 1)) {
        RLU_READER_UNLOCK(p_self);
        return p_parent;
    }

    if(!RLU_TRY_LOCK(p_self, &p_curr_root) || !RLU_TRY_LOCK(p_self, &p_curr)) {
        RLU_ABORT(p_self);
        goto restart;
    }

    p_child = (inner_node_t *)RLU_DEREF(p_self, (p_curr->entry[0].p_inner_child));

    RLU_ASSIGN_PTR(p_self, &(p_curr_root->p_inner), p_child);

    rlu_free_inner(p_self, p_curr);
    rlu_free_inner(p_self, p_next);

    RLU_READER_UNLOCK(p_self);
    return p_curr_root->p_inner;
}

inner_node_t *pure_root_level_up(single_thread_data_t *p_data, int parent_level, int option) {
    return NULL;
}

inner_node_t *rlu_root_level_up(rlu_multi_thread_data_t *p_data, int parent_level, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_root_level_up");
        exit(1);
    }

    root_node_t *p_curr_root;
    inner_node_t *p_curr, *p_new, *p_tail;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_curr = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));

    if(!RLU_TRY_LOCK(p_self, &p_curr_root) || !RLU_TRY_LOCK(p_self, &p_curr)) {
        RLU_ABORT(p_self);
        goto restart;
    }

    p_tail = rlu_new_inner();
    p_tail->state = FREE;
    p_tail->count = 0;
    p_tail->level = parent_level;
    p_tail->min_key = ITEM_MAX_KEY + 1;
    p_tail->p_next = NULL;

    p_new = rlu_new_inner();
    p_new->state = FREE;
    p_new->count = 1;
    p_new->level = parent_level;
    p_new->entry[0].key = p_curr->min_key;
    p_new->min_key = p_new->entry[0].key;

    p_new->p_next = p_tail;

    RLU_ASSIGN_PTR(p_self, &(p_new->entry[0].p_inner_child), p_curr);
    RLU_ASSIGN_PTR(p_self, &(p_curr_root->p_inner), p_new);

    RLU_READER_UNLOCK(p_self);
    return p_new;
}

/////////////////////////////////////////////////////////
// TREE     <OPTION> 2 = SPECIFIC PRINT | 1 = NORMAL PRINT | 0 = NOT PRINT
/////////////////////////////////////////////////////////
// READ
int pure_lb_tree_search(single_thread_data_t *p_data, item_t input, int option) {
    return 1;
}

int rlu_lb_tree_search(rlu_multi_thread_data_t *p_data, item_t input, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_search");
        exit(1);
    }

    int curr_level, entry_number, result;
    root_node_t *p_curr_root;
    inner_node_t *p_inner = NULL;
    leaf_node_t *p_header = NULL;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK (p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, input.key, option);
        if (curr_level > 1) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        }else {
            p_header = (leaf_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_leaf_child));
            curr_level--;
        }
    }

    RLU_READER_UNLOCK (p_self);

    result = rlu_leaf_search(p_data, p_header, input, option);

    if (result == LOST) {
        printf("LB_TREE_SEARCH\n");
        goto restart;
    }

    return result;
}

int pure_lb_tree_range_scan(single_thread_data_t *p_data, int start, int end, int option) {
    return 1;
}

int rlu_lb_tree_range_scan(rlu_multi_thread_data_t *p_data, int start, int end, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_range_scan");
        exit(1);
    }

    int curr_level, entry_number, result;
    root_node_t *p_curr_root;
    inner_node_t *p_inner;
    leaf_node_t *p_header;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    p_header = NULL;
    RLU_READER_LOCK (p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, start, option);
        if (curr_level > 1) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        }else {
            p_header = (leaf_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_leaf_child));
            curr_level--;
        }
    }

    RLU_READER_UNLOCK (p_self);

    result = rlu_leaf_range_scan(p_data, p_header, start, end, option);

    if (result == LOST) {
        printf("LB_TREE_SCAN\n");
        goto restart;
    }

    return result;
}

// WRITE
int pure_lb_tree_add(single_thread_data_t *p_data, item_t input, int option) {
    return 1;
}

int rlu_lb_tree_add(rlu_multi_thread_data_t *p_data, item_t input, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_add");
        exit(1);
    }

    int curr_level, result, entry_number;
    root_node_t *p_curr_root;
    inner_node_t *p_inner;
    leaf_node_t *p_header;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    p_header = NULL;
    RLU_READER_LOCK (p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, input.key, option);
        if (curr_level > 1) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        }else {
            p_header = (leaf_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_leaf_child));
            curr_level--;
        }
    }

    RLU_READER_UNLOCK (p_self);

    result = rlu_leaf_add(p_data, p_header, input, option);
    
    if (result == LOST) {
        printf("LB_TREE_ADD\n");
        goto restart;
    }

    return result;
}

int pure_lb_tree_remove(single_thread_data_t *p_data, item_t input, int option) {
    return 1;
}

int rlu_lb_tree_remove(rlu_multi_thread_data_t *p_data, item_t input, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_remove");
        exit(1);
    }

    int curr_level, result, entry_number;
    root_node_t *p_curr_root;
    inner_node_t *p_inner;
    leaf_node_t *p_header;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    p_header = NULL;
    RLU_READER_LOCK (p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, input.key, option);
        if (curr_level > 1) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        }else {
            p_header = (leaf_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_leaf_child));
            curr_level--;
        }
    }

    RLU_READER_UNLOCK (p_self);

    result = rlu_leaf_remove(p_data, p_header, input, option);

    if (result == LOST) {
        printf("LB_TREE_REMOVE\n");
        goto restart;
    }

    return result;
}

int pure_lb_tree_update(single_thread_data_t *p_data, item_t input, item_t update_input, int option) {
    return 1;
}

int rlu_lb_tree_update(rlu_multi_thread_data_t *p_data, item_t input, item_t update_input, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_update");
        exit(1);
    }

    int curr_level, entry_number, result;
    root_node_t *p_curr_root;
    inner_node_t *p_inner = NULL;
    leaf_node_t *p_header = NULL;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK (p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, input.key, option);
        if (curr_level > 1) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        }else {
            p_header = (leaf_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_leaf_child));
            curr_level--;
        }
    }

    RLU_READER_UNLOCK (p_self);

    result = rlu_leaf_update(p_data, p_header, input, update_input, option);

    if (result == LOST) {goto restart;}

    return result;
}

////////////////////////////////////////////////////////////////
// SPLIT MERGE OPERATOR
////////////////////////////////////////////////////////////////
// NEW      
void pure_new_smo(single_thread_data_t *p_data) {
    return;
}

void rlu_new_smo(rlu_multi_thread_data_t *p_data) {
    if (!p_data) {
        perror("Invalid parameter for pure_new_smo");
        exit(1);
    }
    
    p_data->p_smo = (smo_t *)malloc(sizeof(smo_t));

    if (!p_data->p_smo) {
        perror("Failed to allocate leaf split merge oprator");
        exit(1);
    }
    memset(p_data->p_smo, 0, sizeof(smo_t));
    return;
}

/////////////////////////////////////////////////////////
// HEADER NODE
/////////////////////////////////////////////////////////
// WRITE
int rlu_header_release(rlu_multi_thread_data_t *p_data, int command, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_header_release");
        exit(1);
    }
    
    int operator;
    leaf_node_t *p_master, *p_slave;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    operator = p_data->p_smo->operator;
    RLU_READER_LOCK(p_self);

    if (command == 1) {
        p_master = (leaf_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_header));
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
        p_slave = (leaf_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_header));
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
        p_master = (leaf_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_header));
        p_slave = (leaf_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_header));
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

int pure_header_split(single_thread_data_t *p_data, int option) {
    return 1;
}

int rlu_header_split(rlu_multi_thread_data_t *p_data, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_header_split");
        exit(1);
    }
    
    int i, split_point;
    leaf_node_t *p_header, *p_curr, *p_next, *p_new;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_header));
    p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
    p_header = p_curr;

    split_point = (p_curr->count * lb_meta.distri_r) / 100;

    if (!p_header) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

    for (i = 0; i < (p_header->count - split_point); i++) {
        p_curr = p_next;
        p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));
    }
    
    if (!RLU_TRY_LOCK(p_self, &p_header) || !RLU_TRY_LOCK(p_self, &p_curr) || !RLU_TRY_LOCK(p_self, &p_next)) {
        RLU_ABORT(p_self);
        goto restart;
    }
    
    p_new = rlu_new_leaf();
    p_new->state = SPLIT_REGIST;
    p_new->count = split_point;
    p_new->type = 1;
    p_new->item.key = p_next->item.key;

    p_header->count = p_header->count - split_point;

    RLU_ASSIGN_PTR(p_self, &(p_new->p_next), p_next);
    RLU_ASSIGN_PTR(p_self, &(p_curr->p_next), p_new);

    p_data->p_smo->p_slave_header = p_new;

    RLU_READER_UNLOCK(p_self);
    return SUCCESS;
}

int rlu_header_partner_search(rlu_multi_thread_data_t *p_data, inner_node_t **pp_parent, int parent_level, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_partner_search");
        exit(1);
    }
    int i, master_key, curr_level, entry_number, master_number, slave_number, result;

    root_node_t *p_curr_root;
    inner_node_t *p_inner;
    leaf_node_t *p_regist_header;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    master_key = p_data->p_smo->p_master_header->item.key;

restart:
    entry_number = 0;
    result = 0;
    p_regist_header = NULL;
    
    RLU_READER_LOCK(p_self);
    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level > 0) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, master_key, 10);
        if (curr_level > 1) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        }else {curr_level--;}
    }

    if (p_curr_root->p_inner->level == 1 && p_inner->count == 1) {
        RLU_READER_UNLOCK(p_self);
        return FAIL;
    }

    if (entry_number < p_inner->count - 1) {
        p_regist_header = (leaf_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number + 1].p_leaf_child));
        if (p_regist_header->state == FREE) {result = 1;}
    }

    if (!p_regist_header && entry_number > 0) {
        p_regist_header = (leaf_node_t *)RLU_DEREF(p_self, p_inner->entry[entry_number - 1].p_leaf_child);
        if (p_regist_header->state == FREE) {result = -1;}
    }

    if (result) {
        if(!RLU_TRY_LOCK(p_self, &p_regist_header) || !RLU_TRY_LOCK(p_self, &p_inner)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_regist_header->state = MERGE_REGIST;

        if (result == 1) {
            master_number = entry_number;
            slave_number = entry_number + result;
        } else {
            master_number = entry_number + result;
            slave_number = entry_number;
        }
        
        p_data->p_smo->p_master_header = p_inner->entry[master_number].p_leaf_child;
        p_data->p_smo->p_slave_header = p_inner->entry[slave_number].p_leaf_child;

        for (i = slave_number; i < p_inner->count; i++) {p_inner->entry[i] = p_inner->entry[i + 1];}

        p_inner->count--;

        RLU_ASSIGN_PTR(p_self, pp_parent, p_inner);

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_ABORT(p_self);
        if(entry_number == 0 && p_inner->count == 1) {return FAIL;}
        else {goto restart;}
    }
}

int pure_header_merge(rlu_multi_thread_data_t *p_data, int option) {
    return 1;
}

int rlu_header_merge(rlu_multi_thread_data_t *p_data, int option) {
    if(!p_data) {
        perror("Invalid parameter for rlu_header_merge");
        exit(1);
    }
    
    int result, slave_key;
    leaf_node_t *p_header, *p_prev, *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    slave_key = p_data->p_smo->p_slave_header->item.key;

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_prev = (leaf_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_header));
    p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));
    p_header = p_prev;

    while(1) {
        result = key_compare(slave_key, p_curr->item.key);

        if (result == 2) {
            result = NOTHING;
            break;
        }
        
        if (result == 0 && p_curr->type == 1) { 
            result = SUCCESS;
            break;
        }
        
        p_prev = p_curr;
        p_curr = (leaf_node_t *)RLU_DEREF(p_self, (p_prev->p_next));
        
    }

    if (result) {
        p_next = (leaf_node_t *)RLU_DEREF(p_self, (p_curr->p_next));

        if (!RLU_TRY_LOCK(p_self, &p_header)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if (!RLU_TRY_LOCK(p_self, &p_prev)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if (!RLU_TRY_LOCK(p_self, &p_curr)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_header->count = p_header->count + p_curr->count;

        RLU_ASSIGN_PTR (p_self, &(p_prev->p_next), p_next);

        rlu_free_leaf(p_self, p_curr);

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
int rlu_inner_release(rlu_multi_thread_data_t *p_data, int command, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_inner_release");
        exit(1);
    }
    
    int operator, result;
    inner_node_t *p_master, *p_slave;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    operator = p_data->p_smo->operator;
    RLU_READER_LOCK(p_self);

    if (command == 1) {
        p_master = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
        if(!RLU_TRY_LOCK(p_self, &p_master)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_master->state == MERGE_REGIST) || (operator > 0 && p_master->state == SPLIT_REGIST)) {
            p_master->state = FREE;
            result = SUCCESS;
        }
    } else if (command == 2) {
        p_slave = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_inner));
        if(!RLU_TRY_LOCK(p_self, &p_slave)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if ((operator < 0 && p_slave->state == MERGE_REGIST) || (operator > 0 && p_slave->state == SPLIT_REGIST)) {
            p_slave->state = FREE;
            result = SUCCESS;
        }
    } else {
        p_master = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
        p_slave = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_inner));
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

int rlu_inner_smo_check(rlu_multi_thread_data_t *p_data, inner_node_t *p_master, int command) {
        if(!p_data || !p_master) {return LOST;}

        int split_thres, merge_thres;
        inner_node_t *p_inner;
        rlu_thread_data_t *p_self = p_data->p_rlu_td;

        split_thres = (lb_meta.inner_node_degree * lb_meta.split_thres_r) / 100;
        merge_thres = (lb_meta.inner_node_degree * lb_meta.merge_thres_r) / 100;

restart:
        RLU_READER_LOCK(p_self);

        p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_master));

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

int rlu_inner_entry_devide(rlu_multi_thread_data_t *p_data, inner_node_t *p_master, inner_node_t *p_slave, int option) {
    if (!p_data || !p_master || !p_slave) {return LOST;}

    int i;

    if (p_data->p_smo->operator > 0) {
        for (i = 0; i < p_slave->count; i++) {
            p_slave->entry[i] = p_master->entry[p_master->count + i];
            memset(&p_master->entry[p_master->count + i], 0, sizeof(inner_entry_t));
        }
    }
    return SUCCESS;
}

int rlu_inner_split (rlu_multi_thread_data_t *p_data, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_inner_split");
        exit(1);
    }

    int split_point;
    inner_node_t *p_new, *p_curr, *p_next;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_curr = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
    p_next = (inner_node_t *)RLU_DEREF(p_self, (p_curr->p_next));

    if (!p_curr) {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

    split_point = (p_curr->count * lb_meta.distri_r) / 100;

    if (!RLU_TRY_LOCK(p_self, &p_curr) || !RLU_TRY_LOCK(p_self, &p_next)) {
        RLU_ABORT(p_self);
        goto restart;
    }

    p_new = rlu_new_inner();

    p_new->state = SPLIT_REGIST;
    p_new->count = split_point;
    p_new->level = p_curr->level;

    p_curr->count = p_curr->count - split_point;

    rlu_inner_entry_devide(p_data, p_curr, p_new, option);

    p_new->min_key = p_new->entry[0].key;

    RLU_ASSIGN_PTR(p_self, &(p_new->p_next), p_next);
    RLU_ASSIGN_PTR(p_self, &(p_curr->p_next), p_new);

    p_data->p_smo->p_slave_inner = p_new;

    RLU_READER_UNLOCK(p_self);
    return  SUCCESS;
}

int pure_inner_split_update(single_thread_data_t *p_data, inner_node_t *p_inner, int option) {
    return 1;
}

int rlu_inner_split_update(rlu_multi_thread_data_t *p_data, inner_node_t *p_inner, int option) {
    if (!p_data || !p_inner) { return LOST;}

    int i, m_point, child_key, result;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    result = 0;

    RLU_READER_LOCK(p_self);

    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner));
    
    if (!p_inner) {
        RLU_READER_UNLOCK(p_self);
        return ROLLBACK;
    }

    if (p_inner->level == 1) {
        child_key = p_data->p_smo->p_master_header->item.key;
    } else {
        child_key = p_data->p_smo->p_master_inner->min_key;
    }

    for (m_point = 0; m_point < p_inner->count; m_point++) {
        if (p_inner->entry[m_point].key == child_key) {
            result = 1;
            break;
        }
    }

    if (result) {
        if (!RLU_TRY_LOCK(p_self, &p_inner)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        if (p_inner->count >= lb_meta.inner_node_degree) {
            RLU_ABORT(p_self);
            return ROLLBACK;
        }

        for (i = p_inner->count - 1; i >= m_point + 1; i--) {
            p_inner->entry[i + 1] = p_inner->entry[i];
        }

        if (p_inner->level == 1) {
            p_inner->entry[m_point + 1].key = p_data->p_smo->p_slave_header->item.key;
            p_inner->entry[m_point + 1].p_leaf_child = p_data->p_smo->p_slave_header;
        } else {
            p_inner->entry[m_point + 1].p_inner_child = p_data->p_smo->p_slave_inner;
            p_inner->entry[m_point + 1].key = p_data->p_smo->p_slave_inner->min_key;
        }

        p_inner->count++;

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_READER_UNLOCK(p_self);
        return LOST;
    }

}

int pure_parent_split_update(single_thread_data_t *p_data, int option) {
    return 1;
}

int rlu_parent_split_update(rlu_multi_thread_data_t *p_data, int parent_level, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_parent_split_update");
        exit(1);
    }

    int curr_level, master_key, entry_number, result;
    root_node_t *p_curr_root;
    inner_node_t *p_inner;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    if (parent_level == 1) {master_key = p_data->p_smo->p_master_header->item.key;}
    else {master_key = p_data->p_smo->p_master_inner->min_key;}

restart:
    result = 0;
    RLU_READER_LOCK(p_self);

    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level >= parent_level) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, master_key, option);
        if(curr_level > parent_level) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        } else {break;}
    }

    RLU_ASSIGN_PTR(p_self, &(p_inner), p_inner);

    RLU_READER_UNLOCK(p_self);

    if (curr_level < parent_level) { p_inner = rlu_root_level_up(p_data, parent_level, option);}

    result = rlu_inner_split_update(p_data, p_inner, option);

    if(result != SUCCESS) {goto restart;}

    if (parent_level > 1) {result = rlu_inner_release(p_data, 3, option);}
    else {result = rlu_header_release(p_data, 3, option);}

    if (result == SUCCESS) {rlu_inner_smo_check(p_data, p_inner, 1);}

    return result;

}

int rlu_inner_partner_search(rlu_multi_thread_data_t *p_data, inner_node_t **pp_parent, int parent_level, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_partner_search");
        exit(1);
    }
    int i, master_key, curr_level, entry_number, master_number, slave_number, result;

    root_node_t *p_curr_root;
    inner_node_t *p_inner, *p_regist_inner;
    rlu_thread_data_t *p_self = p_data->p_rlu_td;

    master_key = p_data->p_smo->p_master_inner->min_key;

restart:
    result = 0;
    entry_number = 0;
    p_regist_inner = NULL;
    
    RLU_READER_LOCK(p_self);
    p_curr_root = (root_node_t *)RLU_DEREF(p_self, (p_data->p_root));
    p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_curr_root->p_inner));
    curr_level = p_inner->level;

    while (curr_level >= parent_level) {
        entry_number = rlu_inner_entry_search(p_data, &p_inner, master_key, option);
        if(curr_level > parent_level) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number].p_inner_child));
            curr_level = p_inner->level;
        } else {break;}
    }


    if (entry_number < p_inner->count - 1) {
        p_regist_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number + 1].p_inner_child));
        if (p_regist_inner->state == FREE) {result = 1;}
    }

    if (!p_regist_inner && entry_number > 0) {
        p_regist_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->entry[entry_number - 1].p_inner_child));
        if (p_regist_inner->state == FREE) {result = -1;}
    }

    if (result) {
        if(!RLU_TRY_LOCK(p_self, &p_regist_inner) || !RLU_TRY_LOCK(p_self, &p_inner)) {
            RLU_ABORT(p_self);
            goto restart;
        }

        p_regist_inner->state = MERGE_REGIST;

        if (result == 1) {
            master_number = entry_number;
            slave_number = entry_number + result;
        } else {
            master_number = entry_number + result;
            slave_number = entry_number;
        }

        p_data->p_smo->p_master_inner = p_inner->entry[master_number].p_inner_child;
        p_data->p_smo->p_slave_inner = p_inner->entry[slave_number].p_inner_child;

        for (i = slave_number; i < p_inner->count; i++) {p_inner->entry[i] = p_inner->entry[i + 1];}

        p_inner->count--;

        RLU_ASSIGN_PTR(p_self, pp_parent, p_inner);

        RLU_READER_UNLOCK(p_self);
        return SUCCESS;
    } else {
        RLU_ABORT(p_self);
        goto restart;
    }

}

int pure_inner_merge(single_thread_data_t *p_data, int option) {
    return 1;
}

int rlu_inner_merge(rlu_multi_thread_data_t *p_data, int option) {
     if (!p_data) {
        perror("Invalid parameter for rlu_inner_merge");
        exit(1);
     }

     int i;
     inner_node_t *p_prev, *p_curr, *p_next;
     rlu_thread_data_t *p_self = p_data->p_rlu_td;

restart:
    RLU_READER_LOCK(p_self);

    p_prev = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_master_inner));
    p_curr = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_smo->p_slave_inner));
    
    if (!RLU_TRY_LOCK(p_self, &p_prev) || !RLU_TRY_LOCK(p_self, &p_curr)) {
       RLU_ABORT(p_self);
       goto restart;
    }

    p_next = (inner_node_t *)RLU_DEREF(p_self, (p_curr->p_next));

    for(i = 0; i < p_curr->count; i++) {p_prev->entry[p_prev->count + i] = p_curr->entry[i];}

    p_prev->count = p_prev->count + p_curr->count;

    RLU_ASSIGN_PTR(p_self, &(p_prev->p_next), p_next);

    rlu_free_inner(p_self, p_curr);

    RLU_READER_UNLOCK(p_self);

    return SUCCESS;
}

/////////////////////////////////////////////////////////
// TREE
/////////////////////////////////////////////////////////
// SPLIT
int pure_lb_tree_split(single_thread_data_t *p_data, int option) {
    return 1;
}

int rlu_lb_tree_split(rlu_multi_thread_data_t *p_data, int option) {   
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_split");
        exit(1);
    }

    int result, operator, child_level, parent_level;

    operator = p_data->p_smo->operator;
    child_level = operator % 10;
    parent_level = child_level + 1;

    result = 0;

    if (child_level == 0) {result = rlu_header_split(p_data, option);}
    else {result = rlu_inner_split(p_data, option);}

    if(result) {result = rlu_parent_split_update(p_data, parent_level, option);}

    return result;
}

// MERGE
int pure_lb_tree_merge(single_thread_data_t *p_data, int option) {
    return 1;
}

int rlu_lb_tree_merge(rlu_multi_thread_data_t *p_data, int option) {
    if (!p_data) {
        perror("Invalid parameter for rlu_lb_tree_merge");
        exit(1);
    }

    int result, operator, child_level, parent_level;
    inner_node_t *p_parent_inner;

    operator = p_data->p_smo->operator;
    child_level = abs(operator % 10);
    parent_level = child_level + 1;

    result = 0;

    if (child_level == 0) {
        result = rlu_header_partner_search(p_data, &p_parent_inner, parent_level, option);

        if (result == SUCCESS) {result = rlu_header_merge(p_data, option);}
        else if (result == FAIL) {result = NOTHING;}
        else if (result == LOST) {perror("LOST HEADER IN THE LB TREE MERGE"); exit(1);}

        rlu_header_release(p_data, 1, option);

    } else {
        result = rlu_inner_partner_search(p_data, &p_parent_inner, parent_level, option);

        if (result == SUCCESS) {result = rlu_inner_merge(p_data, option);}
        else if (result == FAIL) {result = NOTHING;}
        else if (result == LOST) {perror("LOST MASTSER IN THE LB TREE MERGE"); exit(1);}

        rlu_inner_release(p_data, 1, option);
    }

    if (result == SUCCESS) {
        p_parent_inner = rlu_root_level_down(p_data, p_parent_inner, option);
        rlu_inner_smo_check(p_data, p_parent_inner, -1);
    }

    return result;
}

////////////////////////////////////////////////////////////////
// PRINT OPERATION
////////////////////////////////////////////////////////////////
// LEAF
void pure_leaf_print(single_thread_data_t *p_data, leaf_node_t *p_leaf, int option) {
    return;
}

void rlu_leaf_print(rlu_multi_thread_data_t* p_data, leaf_node_t *p_leaf, int option) {
    if (!p_data || !p_leaf) {
        perror("Invalid parameter for rlu_leaf_print");
        exit(1);
    }

    switch (p_leaf->type) {
        case 0:
            if (option == 1) {printf("<%d NODE>    : %p \n", p_leaf->item.key, p_leaf);}
            else if (option == 2) {
                printf("----- NORMAL NODE -----\n");
                printf("NODE ADDRESS    : %p \n", p_leaf);
                item_print(p_leaf->item);
            }
            return;
        case 1:
            if (option == 1) {printf("<%d HEADER>     : %p\n", p_leaf->item.key, p_leaf);}
            else {
                printf("----- HEADER NODE -----\n");
                printf("HEADER ADDRESS  : %p\n",p_leaf);
                printf("HEADER STATE    : %d\n",p_leaf->state);
                printf("HEADER COUNT    : %d\n",p_leaf->count);
                printf("HEADER KEY      : %d\n",p_leaf->item.key);
            }
            return;
        default:
            if (option == 1) {printf("<%d TAIL>     : %p\n", p_leaf->item.key, p_leaf);}
            else {
                printf("----- TAIL NODE -----\n");
                printf("TAIL ADDRESS    : %p\n", p_leaf);
                printf("TAIL KEY        : %d\n", p_leaf->item.key);
            }
            return;
    }
}

// INNER
void pure_inner_print(single_thread_data_t *p_data, inner_node_t *p_inner, int option) {
    return;
}

void rlu_inner_print(rlu_multi_thread_data_t* p_data, inner_node_t *p_inner, int option) {
    if (!p_data || !p_inner) {
        perror("Invalid parameter for rlu_inner_print");
        exit(1);
    }

    if (option == 1 && p_inner->count > 0) {
        printf("<key:%d, state:%d>\n", p_inner->min_key, p_inner->state);
    } else if (option == 2 &&  p_inner->count > 0) {
        printf("----- INNER NODE -----\n");
        printf("NODE ADDRESS    : %p\n", p_inner);
        printf("NODE STATE      : %d\n", p_inner->state);
        printf("NODE COUNT      : %d\n", p_inner->count);
        printf("NODE LEVEL      : %d\n", p_inner->level);
        printf("NODE MIN_KEY    : %d\n", p_inner->min_key);
        printf("NODE NEXT       : %p\n", p_inner->p_next);
        printf("_____ CHILD ENTRY _____\n");
        for(int i = 0; i < p_inner->count; i++) {
            printf("[%dth : KEY %d] :", i, (p_inner->entry[i]).key);
            printf("inner_child %p | leaf_child %p", (p_inner->entry[i].p_inner_child), (p_inner->entry[i].p_leaf_child));
            printf("\n");
        }
    } else {
        printf("----- INNER TAIL -----\n");
        printf("NODE ADDRESS    : %p\n", p_inner);
        printf("NODE LEVEL      : %d\n", p_inner->level);
        printf("NODE MIN_KEY    : %d\n", p_inner->min_key);
        printf("NODE NEXT       : %p\n", p_inner->p_next);
    }
}

// ENTIRE TREE
int pure_lb_tree_print(single_thread_data_t *p_data, int start_lv, int end_lv, int option) {
    return 1;
}

int rlu_lb_tree_print(rlu_multi_thread_data_t *p_data, int start_lv, int end_lv, int option) {
    if (!p_data || start_lv < 0 || start_lv < end_lv) {
        perror("Invalid parameter for rlu_lb_tree_print");
        exit(1);
    }

    rlu_thread_data_t *p_self = p_data->p_rlu_td;
    inner_node_t *p_first_inner, *p_inner;
    leaf_node_t *p_leaf;

    RLU_READER_LOCK(p_self);
    
    p_first_inner = (inner_node_t *)RLU_DEREF(p_self, (p_data->p_root->p_inner));
    p_inner = p_first_inner;
    p_leaf = NULL;

    if (start_lv > p_inner->level) { start_lv = p_inner->level;}
    if (end_lv < 0) { end_lv = start_lv - 1;}
    

    while(p_inner->level >= end_lv) {
        rlu_inner_print(p_data, p_inner, option);
        if (p_inner->p_next) {
            p_inner = (inner_node_t *)RLU_DEREF(p_self, (p_inner->p_next));
        } else {
            if (p_inner->level > 1) {
                p_first_inner = (inner_node_t *)RLU_DEREF(p_self, (p_first_inner->entry[0].p_inner_child));
                p_inner = p_first_inner;
            } else {
                p_leaf = (leaf_node_t *) RLU_DEREF(p_self, (p_first_inner->entry[0].p_leaf_child));
                break;
            }
        }
    }

    if (end_lv == 1) {
        RLU_READER_UNLOCK(p_self);
        return 1;
    }

    while(p_leaf) {
        rlu_leaf_print(p_data, p_leaf, 5);
        p_leaf = (leaf_node_t *)RLU_DEREF(p_self, (p_leaf->p_next));
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
        // p_smo_data->p_master_header, p_smo_data->p_slave_header);

}