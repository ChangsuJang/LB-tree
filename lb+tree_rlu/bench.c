////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "bench.h"


////////////////////////////////////////////////////////////////
// BENCHMARK FUNCTIONS
////////////////////////////////////////////////////////////////

void options_meta_set(int argc, char **argv) {
    int i, c;

    struct option long_options[] = {
        {"duration",                        required_argument, NULL, 0},
        {"cycle",                           required_argument, NULL, 0},
        {"add_ratio",                       required_argument, NULL, 0},
        {"remove_ratio",                    required_argument, NULL, 0},
        {"update_ratio",                    required_argument, NULL, 0},
        {"search_ratio",                    required_argument, NULL, 0},
        {"scan_ratio",                      required_argument, NULL, 0},
        {"inner_node_degree",               required_argument, NULL, 0},
        {"leaf_node_degree",                required_argument, NULL, 0},
        {"split_threshold_ratio",           required_argument, NULL, 0},
        {"merge_threshold_ratio",           required_argument, NULL, 0},
        {"distribution_ratio",              required_argument, NULL, 0},
        {"load_tree",                       required_argument, NULL, 0},
        {"nb_threads",                      required_argument, NULL, 0},
        {"rlu_max_ws",                      required_argument, NULL, 0},
        {"base_seed",                       required_argument, NULL, 0},
        {NULL, 0, NULL, 0}
    };

    // DEFALUT SET
    work_meta.duration = DEFAULT_DURATION;
	work_meta.cycle = DEFAULT_CYCLE;
	work_meta.ratio_add = DEFAULT_ADD_RATIO;
	work_meta.ratio_remove = DEFAULT_REMOVE_RATIO;
    work_meta.ratio_update = DEFAULT_UPDATE_RATIO;
	work_meta.ratio_search = DEFAULT_SEARCH_RATIO;
	work_meta.ratio_scan= DEFAULT_SCAN_RATIO;

    lb_meta.inner_node_degree = DEFAULT_TREE_DOCK_DEGREE;
	lb_meta.leaf_node_degree = DEFAULT_NODE_DOCK_DEGREE;
	lb_meta.split_thres_r = DEFAULT_SPLIT_THRESHOLD_RATIO;
	lb_meta.merge_thres_r = DEFAULT_MERGE_THRESHOLD_RATIO;
	lb_meta.distri_r = DEFAULT_DISTRIBUTION_RATIO;
    lb_meta.load_tree = DEFAULT_LOAD_TREE;

    th_meta.nb_threads = DEFAULT_NB_THREADS;
	th_meta.rlu_max_ws = DEFAULT_RLU_MAX_WS;
    th_meta.base_seed = DEFAULT_BASE_SEED;

    for (i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "--bench_set") == 0) {
            bench_set_file = argv[++i];
        } else if (strcmp(argv[i], "--log_option") == 0) {
            log_option = atoi(argv[++i]);
        }
    }
    

    while (1) {
        int file_argc;
        char **file_argv;
        bench_set_file_read(bench_set_file, &file_argc, &file_argv);
        i = 0;
        c = getopt_long(file_argc, file_argv, "", long_options, &i);
        if (c == -1)
            break;
        if (optarg) {
            if (strcmp(long_options[i].name, "duration") == 0) work_meta.duration = atoi(optarg);
            else if (strcmp(long_options[i].name, "cycle") == 0) work_meta.cycle = atoi(optarg);
            else if (strcmp(long_options[i].name, "add_ratio") == 0) work_meta.ratio_add = atoi(optarg);
            else if (strcmp(long_options[i].name, "remove_ratio") == 0) work_meta.ratio_remove = atoi(optarg);
            else if (strcmp(long_options[i].name, "update_ratio") == 0) work_meta.ratio_update = atoi(optarg);
            else if (strcmp(long_options[i].name, "search_ratio") == 0) work_meta.ratio_search = atoi(optarg);
            else if (strcmp(long_options[i].name, "scan_ratio") == 0) work_meta.ratio_scan = atoi(optarg);
            else if (strcmp(long_options[i].name, "inner_node_degree") == 0) lb_meta.inner_node_degree = atoi(optarg);
            else if (strcmp(long_options[i].name, "leaf_node_degree") == 0) lb_meta.leaf_node_degree = atoi(optarg);
            else if (strcmp(long_options[i].name, "split_threshold_ratio") == 0) lb_meta.split_thres_r = atoi(optarg);
            else if (strcmp(long_options[i].name, "merge_threshold_ratio") == 0) lb_meta.merge_thres_r = atoi(optarg);
            else if (strcmp(long_options[i].name, "distribution_ratio") == 0) lb_meta.distri_r = atoi(optarg);
            else if (strcmp(long_options[i].name, "load_tree") == 0) lb_meta.load_tree = atoi(optarg);
            else if (strcmp(long_options[i].name, "nb_threads") == 0) th_meta.nb_threads = atoi(optarg);
            else if (strcmp(long_options[i].name, "rlu_max_ws") == 0) th_meta.rlu_max_ws = atoi(optarg);
            else if (strcmp(long_options[i].name, "base_seed") == 0) th_meta.base_seed = (unsigned short) atoi(optarg);
        }
    }
    
    if(lb_meta.load_tree > -1) {
        char path[50];
        snprintf(path, sizeof(path), "./saved_tree/%d.txt", lb_meta.load_tree);
        load_tree_file = strdup(path);
    }


    #ifdef IS_PURE
        th_meta.nb_threads = 1;
    #else
    #endif // IS_PURE

    if (th_meta.base_seed == 0) {
        th_meta.base_seed = (unsigned short)time(NULL);
    }

    assert(work_meta.duration >= 0);
    assert(work_meta.cycle >= 0);

    assert(lb_meta.inner_node_degree >= 3 && lb_meta.inner_node_degree <= MAX_INNER_DEGREE);
    assert(lb_meta.leaf_node_degree >= 3 && lb_meta.leaf_node_degree <= MAX_LEAF_DEGREE);
    assert(0 < lb_meta.split_thres_r && lb_meta.split_thres_r <=100);
    assert(0 < lb_meta.merge_thres_r && lb_meta.merge_thres_r <= 100);
    assert(0 < lb_meta.distri_r && lb_meta.distri_r <= 100);
    assert(lb_meta.load_tree >= 0);
    assert(work_meta.ratio_add + work_meta.ratio_remove + work_meta.ratio_update + work_meta.ratio_search + work_meta.ratio_scan == 100);

    assert(th_meta.nb_threads > 0);
    assert(th_meta.rlu_max_ws >= 1 && th_meta.rlu_max_ws <= 100);
}

void options_meta_print() {
    printf("----------          WORK LOAD       ----------\n");
    printf("Duration                : %d Milliseconds\n", work_meta.duration);
    printf("Cycles                  : %d\n", work_meta.cycle);
    printf("----------      DATA COMPOSITIOM    ----------\n");
    printf("Ratio of add                 : %d\n", work_meta.ratio_add);
    printf("Ratio of remove              : %d\n", work_meta.ratio_remove);
    printf("Ratio of update              : %d\n", work_meta.ratio_update);
    printf("Ratio of search              : %d\n", work_meta.ratio_search);
    printf("Ratio of scan                : %d\n", work_meta.ratio_scan);
    printf("----------    lb+TREE META DATA     ----------\n");
    printf("Inner node degree       : %d\n", lb_meta.inner_node_degree);
    printf("Leaf node degree        : %d\n", lb_meta.leaf_node_degree);
    printf("Split threshold ratio   : %d\n", lb_meta.split_thres_r);
    printf("Merge threshold ratio   : %d\n", lb_meta.merge_thres_r);
    printf("Distribution ratio      : %d\n", lb_meta.distri_r);
    printf("Nb node anchors         : %d\n", lb_meta.leaf_node_degree);
    printf("Nb tree anchors         : %d\n", lb_meta.inner_node_degree);
    printf("Load tree               : %d\n", lb_meta.load_tree);
    printf("----------   THREADS META DATA      ----------\n");
    printf("Nb threads              : %d\n", th_meta.nb_threads);
    printf("Rlu-max-ws              : %d\n", th_meta.rlu_max_ws);
    printf("Base seed               : %d\n", th_meta.base_seed);
    printf("----------  ITEM TYPE METADATA      ----------\n");
    printf("ITEM KEY                : INT (%d ~ %d)\n", ITEM_MIN_KEY, ITEM_MAX_KEY);
    printf("ITEM VALUE              : INT (%d ~ %d)\n", ITEM_MIN_VALUE, ITEM_MAX_VALUE);
    
	printf("----------         TYPE SIZE        ----------\n");
    printf("System type sizes       :   int=%d    /long=%d    /ptr=%d /word=%d\n",
                                        (int)sizeof(int),
                                        (int)sizeof(long),
                                        (int)sizeof(void *),
                                        (int)sizeof(size_t));
    printf("lb+tree type sizes      :   node=%d /inner_node=%d   /root_node=%d\n",
                                        (int)sizeof(leaf_node_t),
                                        (int)sizeof(inner_node_t),
                                        (int)sizeof(root_node_t));
    printf("Thread type sizes       :   single=%d /rlu=%d\n",
                                        (int)sizeof(single_thread_data_t),
                                        (int)sizeof(rlu_multi_thread_data_t));
    printf("\n\n");
}

void measurements_print() {
    if(work_meta.duration > 0) {
        printf("----------      SET BY RUNNING-TIME         ----------\n");
	    printf("Given   Duration    : %d Milliseconds\n", work_meta.duration);
        printf("Actual  Duration    : %d Milliseconds\n", meas_meta.duration);
        printf("Actual  Cycle       : %d\n", meas_meta.cycle);
    } else {
        printf("----------      SET BY OPERATION-CYCLES     ----------\n");
        printf("Given   Cycle       : %d\n", work_meta.cycle);
        printf("Actual  Cycle       : %d\n", meas_meta.cycle);
        printf("Actual  Duration    : %d Milliseconds\n", meas_meta.duration);
    }
    printf("\n\n");
}

////////////////////////////////////////////////////////////////
// THREADS FUNCTIONS
////////////////////////////////////////////////////////////////
// LOCAL
void local_new_thread(pthread_t **pp_single_thread, pthread_t **pp_rlu_thread,
                        single_thread_data_t **pp_single_data, rlu_multi_thread_data_t **pp_rlu_data) {
#ifdef IS_PURE
    *pp_single_thread = local_new_single_thread();
    *pp_single_data = local_new_single_data();
#else
#ifdef IS_RLU
    *pp_rlu_thread = local_new_rlu_thread();
    *pp_rlu_data = local_new_rlu_data();
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif //IS_PURE
#endif //IS_RLU

}

void local_thread_init(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data) {
#ifdef IS_PURE
#else
#ifdef IS_RLU
    local_rlu_thread_init(p_rlu_data);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif //IS_PURE
#endif //IS_RLU

    return;
}

void local_thread_set(pthread_t *p_single_thread, pthread_t *p_rlu_thread,
                        single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data,
                        barrier_t *p_barrier, pthread_attr_t *p_attr, unsigned short *p_seed, root_node_t* p_root) {
#ifdef IS_PURE
    local_single_thread_set(p_single_thread, p_single_data, p_barrier, p_attr, p_seed, p_root);
#else
#ifdef IS_RLU
    local_rlu_thread_set(p_rlu_thread, p_rlu_data, p_barrier, p_attr, p_seed, p_root);
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif  //IS_PURE
#endif  //IS_RLU
}

void local_thread_wait(pthread_t *p_single_thread, pthread_t *p_rlu_thread) {
#ifdef IS_PURE
    local_single_thread_wait(p_single_thread);
#else
#ifdef IS_RLU
    local_rlu_thread_wait(p_rlu_thread);
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif  //IS_PURE
#endif  //IS_RLU
}
    
void local_thread_finish(single_thread_data_t **pp_single_data, rlu_multi_thread_data_t **pp_rlu_data) {
#ifdef IS_PURE
#else
#ifdef IS_RLU
    local_rlu_thread_finish(*pp_rlu_data);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif //IS_PURE
#endif //IS_RLU
}

void local_free_thread(pthread_t **pp_single_thread, pthread_t **pp_rlu_thread,
                        single_thread_data_t **pp_single_data, rlu_multi_thread_data_t **pp_rlu_data) {
#ifdef IS_PURE
    free(*pp_single_data);
    free(*pp_single_thread);
    *pp_single_data = NULL;
    *pp_single_thread = NULL;
#else
#ifdef IS_RLU
    free(*pp_rlu_data);
    free(*pp_rlu_thread);
    *pp_rlu_data = NULL;
    *pp_rlu_thread = NULL;
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif //IS_PURE
#endif //IS_RLU
}

// GLOBAL
void global_thread_init() {
#ifdef IS_PURE
#else
#ifdef IS_RLU
    RLU_INIT(RLU_TYPE_FINE_GRAINED, th_meta.rlu_max_ws);
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif  //IS_PURE
#endif  //IS_RLU
}

int global_cycle_count(int amount) {
    if (work_meta.duration > 0) {
        return 1;
    }
    int curr_cycle;
    do {
        curr_cycle = atomic_load(&global_cycle);
        if (curr_cycle >= work_meta.cycle) {
            stop = 1;
            return -1;
        }

    } while (!atomic_compare_exchange_weak(&global_cycle, &curr_cycle, curr_cycle + amount));
    
    return (curr_cycle + amount);
}

//  // CONDITION CHECK HELPER
void print_thread_stats() {
	RLU_PRINT_STATS();
    return;
}

////////////////////////////////////////////////////////////////
// lb+TREE FUNCTIONS
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// GENERATE OPRATOR
/////////////////////////////////////////////////////////
root_node_t* new_lb_tree() {
    root_node_t *p_new;
#ifdef IS_PURE
    p_new = pure_new_root();
#else
#ifdef IS_RLU
    p_new = rlu_new_root();
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return p_new;
}

void new_smo(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data) {
#ifdef IS_PURE
    pure_new_smo(p_single_data);
#else
#ifdef IS_RLU
    for(int i = 0; i < th_meta.nb_threads; i++) {
        rlu_new_smo(&p_rlu_data[i]);
    }
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return;
}

/////////////////////////////////////////////////////////
// SERVICE OPERATOR
/////////////////////////////////////////////////////////
int lb_tree_add(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, item_t input, int option) {
    int result;
#ifdef IS_PURE
    result = pure_lb_tree_add(p_single_data, input, option);
#else
#ifdef IS_RLU
    result = rlu_lb_tree_add(p_rlu_data, input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return result;
}

int lb_tree_remove(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, item_t input, int option) {
    int result;
#ifdef IS_PURE
    result = pure_lb_tree_remove(p_single_data, input, option);
#else
#ifdef IS_RLU
    result = rlu_lb_tree_remove(p_rlu_data, input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return result;
}

int lb_tree_update(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, item_t input, item_t update_input, int option) {
    int result;
#ifdef IS_PURE
    result = pure_lb_tree_update(p_single_data, input, update_input, option);
#else
#ifdef IS_RLU
    result = rlu_lb_tree_update(p_rlu_data, input, update_input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return result;
}

int lb_tree_search(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, item_t input, int option) {
    int result;
#ifdef IS_PURE
    result = pure_lb_tree_search(p_single_data, input, option);
#else
#ifdef IS_RLU
    result = rlu_lb_tree_search(p_rlu_data, input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return result;
}

int lb_tree_range_scan(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, int input1, int input2, int option) {
    int result;
#ifdef IS_PURE
    result = pure_lb_tree_range_scan(p_single_data, input1, input2, option);
#else
#ifdef IS_RLU
    result = rlu_lb_tree_range_scan(p_rlu_data, input1, input2, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
    return result;
}

/////////////////////////////////////////////////////////
// SPLIT MERGE OPERATION
/////////////////////////////////////////////////////////

int lb_tree_split(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, int option) {
   int result;
#ifdef IS_PURE
   result = pure_lb_tree_split(p_single_data, option);
#else
#ifdef IS_RLU
   result = rlu_lb_tree_split(p_rlu_data, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
#endif  // IS_PURE
   return result;
}

int lb_tree_merge(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data,  int option) {
    int result;
 #ifdef IS_PURE
    result = pure_lb_tree_merge(p_single_data, option);
 #else
 #ifdef IS_RLU
    result = rlu_lb_tree_merge(p_rlu_data, option);
 #else
     printf("ERROR: benchmark not defined!\n");
     abort();
 #endif  // IS_RLU
 #endif  // IS_PURE
    return result;
 }

/////////////////////////////////////////////////////////
// PRINT OPERATION
/////////////////////////////////////////////////////////
void local_thread_print(single_thread_data_t *p_single, rlu_multi_thread_data_t *p_rlu, int option) {
    #ifdef IS_PURE
        local_single_thread_log_print (p_single, option);
        local_single_thread_print(p_single, option);
    #else
    #ifdef IS_RLU
        local_rlu_thread_log_print (p_rlu, option);
        local_rlu_thread_print(p_rlu, option);
    #else
        printf("ERROR: benchmark not defined!\n");
        abort();
    #endif  //IS_PURE
    #endif  //IS_RLU
    }
    
void lb_tree_print(single_thread_data_t *p_single_data, rlu_multi_thread_data_t *p_rlu_data, int start_lv, int end_lv, int option) {
#ifdef IS_PURE
    pure_lb_tree_print(p_single_data, start_lv, end_lv, option);
#else
#ifdef IS_RLU
    rlu_lb_tree_print(p_rlu_data, start_lv, end_lv, option);
#endif // IS_RLU
#endif // IS_PURE
    return;
}

/////////////////////////////////////////////////////////
// TEST_THREADS
/////////////////////////////////////////////////////////
void* single_test(void *arg) {
/*    
    single_thread_data_t* p_data = (single_thread_data_t *)arg;
    srand(p_data->seed);
    int basic_op_result, sm_op_result, basic_command, start, end;
    item_t item, update_item;

    p_data->local_cycle = 0;
    barrier_cross(p_data->p_barrier);

    while(stop == 0){
        global_cycle_count(1);
        if (stop == 1) {return NULL;}

        basic_command = op_make();
        item = item_make();
    }
   
restart:
        if (p_data->log_option) {
            p_data->p_log[p_data->local_cycle].so_log.operator = basic_command;
            if (basic_command != 5) {p_data->p_log[p_data->local_cycle].so_log.input = item;}
        }
        basic_op_result = 0;
        sm_op_result = 0;
        switch(basic_command) {
            case 1:             //ADD
                p_data->nb_try_add++;
                basic_op_result = lb_tree_add(p_data, NULL, item, 0);
                if (basic_op_result > 0) {
                    p_data->nb_add++;
                    p_data->diff++;
                }
                break;
            case 2:             //REMOVE
                p_data->nb_try_remove++;
                basic_op_result = lb_tree_remove(p_data, NULL, item, 0);
                if (basic_op_result > 0) {
                    p_data->nb_remove++;
                    p_data->diff--;
                }
                break;
            case 3:             //UPDATE
                p_data->nb_try_update++;
                update_item = item_make();
                basic_op_result = lb_tree_update(p_data, NULL, item, update_item, 0);
                if (basic_op_result > 0) {
                    p_data->nb_update++;
                }
                break;
            case 4:             //SEARCH
                p_data->nb_try_search++;
                basic_op_result = lb_tree_search(p_data, NULL, item, 0);
                if (basic_op_result > 0) {
                    p_data->nb_search++;
                }
                break;
                case 5:
                p_data->nb_try_scan++;
                start = rand() % (ITEM_MAX_KEY - ITEM_MIN_KEY) + ITEM_MIN_KEY;  
                end = rand() % (ITEM_MAX_KEY - start) + (start + 1);
                basic_op_result = lb_tree_range_scan(p_data, NULL, start, end, 2);
                if (basic_op_result > 0) {
                    p_data->nb_scan++;
                }
                break;
        }

        if (basic_op_result < -1) {goto restart;}
 
        while (1) {
            temp = p_data->p_smo->operator;
            switch(temp) {
                case -10:
                    sm_op_result = lb_tree_merge(p_data, NULL, 0);
                    if (sm_op_result > 0) {
                        p_data->nb_header_merge++;
                    }
                    break;
                case 10:
                    sm_op_result = lb_tree_split(p_data, NULL, 0);
                    if (sm_op_result > 0) {
                        p_data->nb_header_split++;
                    }
                    break;
                default :
                    break;
            }
            if (temp == p_data->p_smo->operator) {
                memset(p_data->p_smo, 0, sizeof(smo_t));
                break;
            }
        }

        if (p_data->log_option) {
            if (basic_command == 3) {p_data->p_log[p_data->local_cycle].so_log.update_input = update_item;}
            if (basic_command == 5) {
                p_data->p_log[p_data->local_cycle].so_log.start_key = start;
                p_data->p_log[p_data->local_cycle].so_log.end_key = end;
            }
            p_data->p_log[p_data->local_cycle].so_log.result = basic_op_result;
            p_data->p_log[p_data->local_cycle].smo_log.result = sm_op_result;
        }
        p_data->local_cycle++;
    }
*/
    return NULL;
}

void* rlu_test(void *arg) {
    rlu_multi_thread_data_t* p_data = (rlu_multi_thread_data_t *)arg;
    srand(p_data->seed);
    int basic_op_result, sm_op_result, basic_command, start, end, temp;
    item_t item, update_item;

    p_data->local_cycle = 0;

    barrier_cross(p_data->p_barrier);

    while(stop == 0){
        temp = global_cycle_count(1);
        if (temp == -1) { break;}

        basic_command = op_make();
        item = item_make();
        
        if (p_data->log_option) {
            p_data->p_log[p_data->local_cycle].start_g_cycle = temp;
            p_data->p_log[p_data->local_cycle].so_log.operator = basic_command;
            if (basic_command != 5) {p_data->p_log[p_data->local_cycle].so_log.input = item;}
        }
        basic_op_result = 0;
        sm_op_result = 0;
        if (p_data->p_smo->operator == 0) {
            switch(basic_command) {
                case 1:
                    p_data->nb_try_add++;
                    basic_op_result = lb_tree_add(NULL, p_data, item, 2);
                    if (basic_op_result > 0) {
                        p_data->nb_add++;
                        p_data->diff++;
                    }
                    break;
                case 2:
                    p_data->nb_try_remove++;
                    basic_op_result = lb_tree_remove(NULL, p_data, item, 2);
                    if (basic_op_result > 0) {
                        p_data->nb_remove++;
                        p_data->diff--;
                    }
                    break;
                case 3:
                    p_data->nb_try_update++;
                    update_item = item_make();
                    basic_op_result = lb_tree_update(NULL, p_data, item, update_item, 2);
                    if (basic_op_result > 0) {
                        p_data->nb_update++;
                    }
                   break;
                case 4:
                   p_data->nb_try_search++;
                   basic_op_result = lb_tree_search(NULL, p_data, item, 2);
                   if (basic_op_result > 0) {
                       p_data->nb_search++;
                   }
                   break;
                case 5:
                    p_data->nb_try_scan++;   
                    start = rand() % (ITEM_MAX_KEY - ITEM_MIN_KEY) + ITEM_MIN_KEY;  
                    end = rand() % (ITEM_MAX_KEY - start) + (start + 1);
                    basic_op_result = lb_tree_range_scan(NULL, p_data, start, end, 2);
                    break;
            }
        } else {
            temp = p_data->p_smo->operator;
            if (temp > 0) {
                if (temp == 10) {p_data->nb_try_header_split++;}
                else {p_data->nb_try_inner_split++;}
                sm_op_result = lb_tree_split(NULL, p_data, 0);
                if (sm_op_result) {
                    if (temp == 10) {p_data->nb_header_split++;}
                    else {p_data->nb_inner_split++;}
                }
            } else {
                if (temp == -10) {p_data->nb_try_header_merge++;}
                else {p_data->nb_try_inner_merge++;}
                sm_op_result = lb_tree_merge(NULL, p_data, 0);
                if (sm_op_result) {
                    if (temp == -10) {p_data->nb_header_merge++;}
                    else {p_data->nb_inner_merge++;}
                }
            }
            if (temp == p_data->p_smo->operator) {
                memset(p_data->p_smo, 0, sizeof(smo_t));
            }
        }
        
        if (p_data->log_option) {
            if (basic_command == 3) {p_data->p_log[p_data->local_cycle].so_log.update_input = update_item;}
            p_data->p_log[p_data->local_cycle].so_log.result = basic_op_result;
            p_data->p_log[p_data->local_cycle].end_g_cycle = 1;
            p_data->p_log[p_data->local_cycle].smo_log.result = sm_op_result;
        }
        if (p_data->local_cycle > 90000) {p_data->local_cycle = 0;}
        else {p_data->local_cycle++;}
    }
    return NULL;
}

/////////////////////////////////////////////////////////
// SIGNAL HANDLING
/////////////////////////////////////////////////////////
void trigger_print() {
    // measurements_print();
        
    // local_thread_print(g_p_single, g_p_rlu, 2);

    // lb_tree_print(g_p_single, g_p_rlu, 5, 0, 2);

    // print_thread_stats();
}

void segfault_handler(int sig) {
    fprintf(stderr, "Error: signal %d:\n", sig);

    trigger_print();

    exit(1);
}

void forced_termination_handler (int sig) {
    if (sig == SIGINT) {
        trigger_print();
        exit(1);
    }
}

/////////////////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////////////////
int main(int argc, char **argv) {
// BENCHMARK SET
    printf("\n\n==========      START BENCHMARK        ==========\n\n");

    options_meta_set(argc, argv);

    printf("\n\n==========      SETTING TEST        ==========\n\n");

    options_meta_print();

    struct sigaction sa;

    unsigned short *p_thread_seed;

    struct timeval start, end;
	struct timespec timeout;

    pthread_attr_t attr;
    barrier_t barrier;

    pthread_t *p_single_thread, *p_rlu_thread;
    single_thread_data_t *p_single_data;
    rlu_multi_thread_data_t *p_rlu_data;

    root_node_t *p_root;

    local_new_thread(&p_single_thread, &p_rlu_thread,
                        &p_single_data, &p_rlu_data);
    
    g_p_single = p_single_data;
    g_p_rlu = p_rlu_data;

    p_root = new_lb_tree();

    g_p_root = p_root;

    new_smo(p_single_data, p_rlu_data);

    new_rand(base_seed, &p_thread_seed);

    sa.sa_handler = segfault_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    signal(SIGINT, forced_termination_handler);

    timeout.tv_sec = work_meta.duration / 1000;
    timeout.tv_nsec = (work_meta.duration % 1000) * 1000000;

    barrier_init(&barrier, th_meta.nb_threads+1);
    pthread_attr_init(&attr);

    global_thread_init();
    local_thread_init(p_single_data, p_rlu_data);

    
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    local_thread_set(p_single_thread, p_rlu_thread, p_single_data, p_rlu_data,
                        &barrier, &attr, p_thread_seed, p_root);

    pthread_attr_destroy(&attr);

    stop = 0;
    global_cycle = 0;
    barrier_cross(&barrier);

    printf("\n\n==========      STARTING THREADS        ==========\n\n");

    gettimeofday(&start, NULL);
    if (work_meta.duration > 0) {
        nanosleep(&timeout, NULL);
        stop = 1;
    }
    
    local_thread_wait(p_single_thread, p_rlu_thread);

    gettimeofday(&end, NULL);
    printf("\n\n==========      STOPPING THREADS        ==========\n\n");


    meas_meta.duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
    meas_meta.cycle = global_cycle;
    
    local_thread_finish(&p_single_data, &p_rlu_data);

    printf("\n\n==========      PRINT MEASUREMENTS        ==========\n\n"); 
    
    local_thread_print(p_single_data, p_rlu_data, 2);

    // lb_tree_print(p_single_data, p_rlu_data, 10, -1, 2);

    measurements_print();

    local_free_thread(&p_single_thread, &p_rlu_thread, &p_single_data, &p_rlu_data);
    free_rand(p_thread_seed);

    print_thread_stats();

    printf("\n\n==========      FINISH BENCHMARK        ==========\n\n");
	return 0;
}
