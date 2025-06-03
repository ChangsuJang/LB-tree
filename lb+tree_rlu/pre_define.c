////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "pre_define.h"

////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////
// DATA                         //
// TYPE 0
item_t const min_item = {ITEM_MIN_KEY, ITEM_MIN_VALUE};
item_t const max_item = {ITEM_MAX_KEY, ITEM_MAX_VALUE};

hardware_meta_t hard_meta;
workloads_meta_t work_meta;
measurements_meta_t meas_meta;
lb_tree_meta_t lb_meta;
threads_meta_t th_meta;

char *bench_set_file;
char *load_tree_file;
int log_option;

////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////
// READ OUTSOURCE       //
void bench_set_file_read(const char *filename, int *argc, char ***argv) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    *argc = 0;          // Initialize argc
    *argv = NULL;       // Initialize argv to NULL

    while (fgets(line, sizeof(line), file)) {
        // Tokenize the line into an array of arguments
        char *token = strtok(line, " \t\n");
        while (token != NULL) {
            (*argc)++;
            *argv = realloc(*argv, (*argc) * sizeof(char *));
            if (*argv == NULL) {
                perror("Failed to realloc memory");
                exit(EXIT_FAILURE);
            }
            (*argv)[(*argc) - 1] = strdup(token);
            if ((*argv)[(*argc) - 1] == NULL) {
                perror("Failed to duplicate string");
                exit(EXIT_FAILURE);
            }
            token = strtok(NULL, " \t\n");
        }
    }
    fclose(file);
    return;
}

// RANDOM SEED          //
inline void new_rand(unsigned short src_seed, unsigned short **obj_seed) {
    srand(src_seed);
    *obj_seed = (unsigned short*)malloc(th_meta.nb_threads * sizeof(unsigned short));
    for (int i = 0; i < th_meta.nb_threads; i++) {
        (*obj_seed)[i] = rand();
    }
}

inline void free_rand(unsigned short *seed) {
    free(seed);
    seed = NULL;
}
////////////////////////////////////////////////////////////////
// OPERATION GENERATOR FUNCTIONS
////////////////////////////////////////////////////////////////
// ITEM                         //
int op_make() {

    int r = rand() % 100 + 1;
    int stack = 0;

    stack = work_meta.ratio_add;
    // printf("r %d | stack %d\n", r,stack);
    if (r <= stack) return 1;
    stack = stack + work_meta.ratio_remove;
    if (r <= stack) return 2;
    stack = stack + work_meta.ratio_update;
    if (r <= stack) return 3;
    stack = stack + work_meta.ratio_search;
    if (r <= stack) return 4;
    stack = stack + work_meta.ratio_scan;
    if (r <= stack) return 5;

    return 0;
}

item_t item_make() {
    item_t new;
    new.key = rand() % (ITEM_MAX_KEY - ITEM_MIN_KEY + 1) + ITEM_MIN_KEY;
    new.value = rand() % (ITEM_MAX_VALUE - ITEM_MIN_VALUE + 1) + ITEM_MIN_VALUE;
    return new;
}

inline int key_compare(int src, int obj) {
    if (src > obj) {
        return 1;
    } else if (src == obj) {
        return 0;
    } else {
        return 2;
    }
}
////////////////////////////////////////////////////////////////
// THREADS FUNCTIONS
////////////////////////////////////////////////////////////////

// BARRIER              //
void barrier_init(barrier_t *b, int n) {
  pthread_cond_init(&b->complete, NULL);
  pthread_mutex_init(&b->mutex, NULL);
  b->count = n;
  b->crossing = 0;
}

void barrier_cross(barrier_t *b) {
  pthread_mutex_lock(&b->mutex);
  /* One more thread through */
  b->crossing++;
  /* If not all here, wait */
  if (b->crossing < b->count) {
    pthread_cond_wait(&b->complete, &b->mutex);
  } else {
    pthread_cond_broadcast(&b->complete);
    /* Reset for next time */
    b->crossing = 0;
  }
  pthread_mutex_unlock(&b->mutex);
}

// GLOBAL               //

// LOCAL - SINGLE       //

pthread_t *local_new_single_thread() {
    pthread_t *p_single_thread;

    p_single_thread = (pthread_t *)malloc(sizeof(pthread_t));

    if(!p_single_thread) {
        perror("single thread pthread malloc");
        exit(1);
    }

    return p_single_thread;
}

single_thread_data_t *local_new_single_data() {
    single_thread_data_t *p_single_data;

    p_single_data = (single_thread_data_t *)malloc(sizeof(single_thread_data_t));

    if (!p_single_data) {
        perror("single thread data malloc");
        exit(1);
    }

    memset(p_single_data, 0, sizeof(single_thread_data_t));

    return p_single_data;
}

single_thread_log_t *local_new_single_thread_log() {
    single_thread_log_t *p_single_log;

    if (!work_meta.duration) {
        p_single_log = (single_thread_log_t *)malloc(work_meta.cycle * sizeof(single_thread_log_t));
        memset(p_single_log, 0, work_meta.cycle * sizeof(single_thread_log_t));
    } else {
        p_single_log = (single_thread_log_t *)malloc(100000 * sizeof(single_thread_log_t));
        memset(p_single_log, 0, 100000 * sizeof(single_thread_log_t));
    }

    if (!p_single_log) {
        perror("Failed to allocate log for local_new_single_thread_log");
        exit(1);
    }

    return p_single_log;
}

void local_single_thread_set(pthread_t* p_thread, single_thread_data_t *p_data, barrier_t *p_barrier, pthread_attr_t *p_attr, unsigned short *p_seed, root_node_t* p_root) {

    if(p_data == NULL) {
        perror("single thread is empty");
        exit(1);
    }
    p_data->seed = *p_seed;
    p_data->p_barrier = p_barrier;
    p_data->p_root = p_root;
    p_data->log_option = log_option;
    if (p_data->log_option) {p_data->p_log = local_new_single_thread_log();}

    if(pthread_create(p_thread, p_attr, single_test, (void *)p_data) != 0) {
        fprintf(stderr, "Error creating single thread\n");
        exit(1);
    }

    return;
}

void local_single_thread_wait(pthread_t *p_thread) {
    pthread_join(*p_thread, NULL);
}

// LOCAL - RLU      //
pthread_t *local_new_rlu_thread() {
    pthread_t *p_rlu_threads;

    p_rlu_threads = (pthread_t *)malloc(th_meta.nb_threads * sizeof(pthread_t));

    if(p_rlu_threads == NULL) {
        perror("rlu threads pthread malloc");
        exit(1);
    }

    return p_rlu_threads;
}

rlu_multi_thread_data_t *local_new_rlu_data() {
    
    rlu_multi_thread_data_t *p_rlu_data;
    p_rlu_data = (rlu_multi_thread_data_t *)malloc(th_meta.nb_threads * sizeof(rlu_multi_thread_data_t));

    if(p_rlu_data == NULL) {
        perror("rlu thread data malloc");
        exit(1);
    }

    memset(p_rlu_data, 0, th_meta.nb_threads * sizeof(rlu_multi_thread_data_t));
    return p_rlu_data;
}

rlu_multi_thread_log_t *local_new_rlu_thread_log() {
    rlu_multi_thread_log_t *p_rlu_multi_log;

    if (!work_meta.duration) {
        p_rlu_multi_log = (rlu_multi_thread_log_t *)malloc(work_meta.cycle * sizeof(rlu_multi_thread_log_t));
        memset(p_rlu_multi_log, 0, work_meta.cycle * sizeof(rlu_multi_thread_log_t));
    } else {
        p_rlu_multi_log = (rlu_multi_thread_log_t *)malloc(100000 * sizeof(rlu_multi_thread_log_t));
        memset(p_rlu_multi_log, 0, 100000 * sizeof(rlu_multi_thread_log_t));
    }
    
    if (!p_rlu_multi_log) {
        perror("Failed to allocate log for local_new_single_thread_log");
        exit(1);
    }
    
    return p_rlu_multi_log;
}

void local_rlu_thread_init(rlu_multi_thread_data_t *p_data) {
    for(int i = 0; i < th_meta.nb_threads; i++){
        p_data[i].uniq_id = i;
        p_data[i].p_rlu_td = &(p_data[i].rlu_td);
        RLU_THREAD_INIT(p_data[i].p_rlu_td);   
    }
    return;
}

void local_rlu_thread_set(pthread_t *p_thread, rlu_multi_thread_data_t *p_data, barrier_t *p_barrier, pthread_attr_t *p_attr, unsigned short *p_seed, root_node_t* p_root) {
    if(p_data == NULL) {
        perror("service thread is empty");
        exit(1);
    }

    for (int i = 0; i < th_meta.nb_threads; i++) {
        p_data[i].seed = p_seed[p_data[i].uniq_id];
        p_data[i].p_barrier = p_barrier;
        p_data[i].p_root = p_root;
        p_data[i].log_option = log_option;
        if (p_data[i].log_option) {p_data[i].p_log = local_new_rlu_thread_log();}
        if (pthread_create(&p_thread[i], p_attr, rlu_test, (void *)&p_data[i]) != 0) {
            fprintf(stderr, "Error creating service thread\n");
            exit(1);
        }
    }
    return;
}

void local_rlu_thread_wait(pthread_t *p_thread) {
    for (int i = 0; i < th_meta.nb_threads; i++) {
        pthread_join(p_thread[i], NULL);
    }
}

void local_rlu_thread_finish(rlu_multi_thread_data_t *p_data){
    for(int i = 0; i < th_meta.nb_threads; i++){
        RLU_THREAD_FINISH(p_data[i].p_rlu_td);
    }
}

////////////////////////////////////////////////////////////////
// PRINT FUNCTION
////////////////////////////////////////////////////////////////

void item_print(item_t src) {
    printf("KEY : %d    ", src.key);
    printf("VALUE : %d\n", src.value);
}

int so_log_print(so_log_t so_log, int option) {
    if (so_log.operator == 0) {return -1;}

    switch (so_log.operator) {
        case 1:
            printf("    ADD ");
            break;
        case 2:
            printf(" REMOVE ");
            break;
        case 3:
            printf(" UPDATE ");
            break;
        case 4:
            printf(" SEARCH ");
            break;
        case 5:
            printf("   SCAN ");
            break;
        default:
            printf("    %d", so_log.operator);
            break;
    }

    switch (so_log.result) {
        case 0:
            printf("FAIL    : ");
            break;
        case 1:
            printf("SUCCESS : ");
            break;
    }

    if (so_log.operator < 5) {
        item_print(so_log.input);
        if (so_log.operator == 3) {
            printf("                  ");
            item_print(so_log.update_input);
        }
    } else {
        printf(" START KEY : %d  ~  END KEY : %d\n", so_log.start_key, so_log.end_key);
    }
    
    if (!option) {return 1;}

    printf("HEADER KEY LOG   :  %d  |  %d  |  %d  \n", so_log.header_key[0], so_log.header_key[1], so_log.header_key[2]);
    printf("HEADER STATE LOG :  %d  |  %d  |  %d  \n", so_log.header_state[0], so_log.header_state[1], so_log.header_state[2]);
    printf("HEADER COUNT LOG :  %d  |  %d  |  %d  \n", so_log.header_count[0], so_log.header_count[1], so_log.header_count[2]);

    return 1;
}

int smo_log_print(smo_log_t smo_log, int option) {
    if (smo_log.operator == 0) {return -1;}
    if (smo_log.operator > 0 || smo_log.result == 1) {return 1;}
    if (smo_log.operator == -10) {return 1;}

    int level = (smo_log.operator % 10);

    printf("\n<");

    if (level == 0) {
        printf("    LEAF");
    } else {
        printf("%d INNER", abs(level));
    }

    if (smo_log.operator > 0) {
        printf(" SPLIT ");
    } else {
        printf(" MERGE ");
    }

    switch (smo_log.result) {
        case -1:
            printf("FAIL  ");
            break;
        case 0:
            printf("NOTHING        ");
            break;
        case 1:
            printf("SUCCESS     ");
            break;
    }
    printf(">\n");
    
    level = smo_log.operator % 10;

    if (option == 1) {return 1;}

    printf("PHASE 1\n");
    printf("MASTER_HEADER   :  KEY %d \n", smo_log.m_header_key[0]);
    printf("SLAVE_HEADER    :  KEY %d \n", smo_log.s_header_key[0]);
    printf("MASTER_INNER    :  KEY %d \n", smo_log.m_inner_key[0]);
    printf("SLAVE_INNER     :  KEY %d \n", smo_log.s_inner_key[0]);
    printf("PHASE 2\n");
    printf("MASTER_HEADER   :  KEY %d \n", smo_log.m_header_key[1]);
    printf("SLAVE_HEADER    :  KEY %d \n", smo_log.s_header_key[1]);
    printf("MASTER_INNER    :  KEY %d \n", smo_log.m_inner_key[1]);
    printf("SLAVE_INNER     :  KEY %d \n", smo_log.s_inner_key[1]);
    printf("PHASE 3\n");
    printf("MASTER_HEADER   :  KEY %d \n", smo_log.m_header_key[2]);
    printf("SLAVE_HEADER    :  KEY %d \n", smo_log.s_header_key[2]);
    printf("MASTER_INNER    :  KEY %d \n", smo_log.m_inner_key[2]);
    printf("SLAVE_INNER     :  KEY %d \n", smo_log.s_inner_key[2]);
    printf("\n");
    return 1;
}

void local_single_thread_print(single_thread_data_t *p_data, int option) {
    printf("\n---------- SINGLE THREAD 0 ----------\n");
    printf("The number of TRY ADD           : %d\n", p_data->nb_try_add);
    printf("The number of ADD               : %d\n", p_data->nb_add);
    printf("The number of TRY REMOVE        : %d\n", p_data->nb_try_remove);
    printf("The number of REMOVE            : %d\n", p_data->nb_remove);
    printf("The number of TRY UPDATE        : %d\n", p_data->nb_try_update);
    printf("The number of UPDATE            : %d\n", p_data->nb_update);
    printf("The number of TRY SEARCH        : %d\n", p_data->nb_try_search);
    printf("The number of SEARCH            : %d\n", p_data->nb_search);
    printf("The number of TRY SCAN          : %d\n", p_data->nb_try_scan);
    printf("The number of SCAN              : %d\n", p_data->nb_scan);
    printf("The number of TRY TREE SPLIT    : %d\n", p_data->nb_try_header_split);
    printf("The number of TREE SPLIT        : %d\n", p_data->nb_header_split);
    printf("The number of TRY TREE MERGE    : %d\n", p_data->nb_try_header_merge);
    printf("The number of TREE MERGE        : %d\n", p_data->nb_header_merge);
    printf("Modified size by Thread         : %d\n", p_data->diff);
}

void local_single_thread_log_print(single_thread_data_t *p_data, int option) {
    if(!p_data) {
        perror("Invalid parameter for pure_so_print");
        exit(1);
    }

    if(!p_data->log_option) {return;}

    int i, so_result, smo_result;

    i = 0;

    while(1) {
        printf ("============ # %d log ============ \n", i+1);
        so_result = so_log_print (p_data->p_log[i].so_log, option);
        smo_result = smo_log_print (p_data->p_log[i].smo_log, option);

        if(so_result == -1 && smo_result == -1) {return;}

        i++;
    }
}

void local_rlu_thread_print(rlu_multi_thread_data_t *p_data, int option) {
    unsigned int total_try_add = 0, total_add = 0;
    unsigned int total_try_remove = 0, total_remove = 0;
    unsigned int total_try_update = 0, total_update = 0;
    unsigned int total_try_search = 0, total_search = 0;
    unsigned int total_try_scan = 0, total_scan = 0;
    unsigned int total_diff = 0;
    unsigned int total_try_header_split = 0, total_header_split = 0;
    unsigned int total_try_header_merge = 0, total_header_merge = 0;
    unsigned int total_try_inner_split = 0, total_inner_split = 0;
    unsigned int total_try_inner_merge = 0, total_inner_merge = 0;

    for (int i = 0; i < th_meta.nb_threads; i++) {
        printf("\n---------- THREAD %ld ----------\n", p_data[i].uniq_id);
        printf("The number of TRY ADD           : %d\n", p_data[i].nb_try_add);
        printf("The number of ADD               : %d\n", p_data[i].nb_add);
        printf("The number of TRY REMOVE        : %d\n", p_data[i].nb_try_remove);
        printf("The number of REMOVE            : %d\n", p_data[i].nb_remove);
        printf("The number of TRY UPDATE        : %d\n", p_data[i].nb_try_update);
        printf("The number of UPDATE            : %d\n", p_data[i].nb_update);
        printf("The number of TRY SEARCH        : %d\n", p_data[i].nb_try_search);
        printf("The number of SEARCH            : %d\n", p_data[i].nb_search);
        printf("The number of TRY SCAN          : %d\n", p_data[i].nb_try_scan);
        printf("The number of SCAN              : %d\n", p_data[i].nb_scan);
        printf("The number of TRY HEADER SPLIT  : %d\n", p_data[i].nb_try_header_split);
        printf("The number of HEADER SPLIT      : %d\n", p_data[i].nb_header_split);
        printf("The number of TRY HEADER MERGE  : %d\n", p_data[i].nb_try_header_merge);
        printf("The number of HEADER MERGE      : %d\n", p_data[i].nb_header_merge);
        printf("The number of TRY INNER SPLIT   : %d\n", p_data[i].nb_try_inner_split);
        printf("The number of INNER SPLIT       : %d\n", p_data[i].nb_inner_split);
        printf("The number of TRY INNER MERGE   : %d\n", p_data[i].nb_try_inner_merge);
        printf("The number of INNER MERGE       : %d\n", p_data[i].nb_inner_merge);
        printf("Modified size by Thread         : %d\n", p_data[i].diff);

        total_try_add += p_data[i].nb_try_add;
        total_add += p_data[i].nb_add;
        total_try_remove += p_data[i].nb_try_remove;
        total_remove += p_data[i].nb_remove;
        total_try_update += p_data[i].nb_try_update;
        total_update += p_data[i].nb_update;
        total_try_search += p_data[i].nb_try_search;
        total_search += p_data[i].nb_search;
        total_try_scan += p_data[i].nb_try_scan;
        total_scan += p_data[i].nb_scan;
        total_try_header_split += p_data[i].nb_try_header_split;
        total_header_split += p_data[i].nb_header_split;
        total_try_header_merge += p_data[i].nb_try_header_merge;
        total_header_merge += p_data[i].nb_header_merge;
        total_try_inner_split += p_data[i].nb_try_inner_split;
        total_inner_split += p_data[i].nb_inner_split;
        total_try_inner_merge += p_data[i].nb_try_inner_merge;
        total_inner_merge += p_data[i].nb_inner_merge;
        total_diff += p_data[i].diff;
    }

    printf("\n---------- TOTAL THREAD ----------\n");
    printf("Total TRY ADD           : %u\n", total_try_add);
    printf("Total ADD               : %u\n", total_add);
    printf("Total TRY REMOVE        : %u\n", total_try_remove);
    printf("Total REMOVE            : %u\n", total_remove);
    printf("Total TRY UPDATE        : %u\n", total_try_update);
    printf("Total UPDATE            : %u\n", total_update);
    printf("Total TRY SEARCH        : %u\n", total_try_search);
    printf("Total SEARCH            : %u\n", total_search);
    printf("Total TRY SCAN          : %u\n", total_try_scan);
    printf("Total SCAN              : %u\n", total_scan);
    printf("Total TRY HEADER SPLIT  : %u\n", total_try_header_split);
    printf("Total HEADER SPLIT      : %u\n", total_header_split);
    printf("Total TRY HEADER MERGE  : %u\n", total_try_header_merge);
    printf("Total HEADER MERGE      : %u\n", total_header_merge);
    printf("Total TRY INNER SPLIT   : %u\n", total_try_inner_split);
    printf("Total INNER SPLIT       : %u\n", total_inner_split);
    printf("Total TRY INNER MERGE   : %u\n", total_try_inner_merge);
    printf("Total INNER MERGE       : %u\n", total_inner_merge);
    printf("Total Modified Size     : %u\n", total_diff);
}


void local_rlu_thread_log_print(rlu_multi_thread_data_t *p_data, int option) {
    if (!p_data) {
        perror("Invalid parameter for pure_so_print");
        exit(1);
    }

    if (!p_data->log_option) { return; }

    int i, j, so_result, smo_result;

    for (i = 0; i < th_meta.nb_threads; i++) {
        int log_count = 0;
        while (p_data[i].p_log[log_count].start_g_cycle != 0) {
            smo_result = smo_log_print(p_data[i].p_log[log_count].smo_log, option);
            log_count++;
        }
        
        int start_index = (log_count > 10) ? log_count - 10 : 0;

        for (j = start_index; j < log_count; j++) {
            printf("============ THREAD %ld #%d log ============ \n", p_data[i].uniq_id, j + 1);
            printf("Global Cycle Start capture : %d\n", p_data[i].p_log[j].start_g_cycle);
            so_result = so_log_print(p_data[i].p_log[j].so_log, option);
            smo_result = smo_log_print(p_data[i].p_log[j].smo_log, option);
            printf("Global Cycle End capture   : %d\n", p_data[i].p_log[j].end_g_cycle);

            if (so_result == -1 && smo_result == -1) { return; }
        }
    }
}
