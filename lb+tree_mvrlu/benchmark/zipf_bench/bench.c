////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "lb_tree.h"
#include "pre_define.h"

#include "numa-config.h"
#include "zipf/zipf.h"

////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////
// WORK LOADS				//
#define DEFAULT_DURATION			10000
#define DEFAULT_KEY_RANGE           MAX_KEY
#define DEFAULT_INIT_SIZE           500
#define DEFAULT_ZIPF                0
#define DEFAULT_ZIPF_DIST_VAL       0.0

// WORK LOADS DISTRI		//
#define DEFAULT_ADD_RATIO			30
#define DEFAULT_REMOVE_RATIO		20
#define DEFAULT_SEARCH_RATIO		45
#define DEFAULT_SCAN_RATIO			5

// HB+TREE_COND				//
#define DEFAULT_INNER_DEGREE  		4
#define DEFAULT_LEAF_DEGREE			10
#define DEFAULT_SPLIT_THRESHOLD_RATIO		100
#define DEFAULT_MERGE_THRESHOLD_RATIO		25
#define DEFAULT_DISTRIBUTION_RATIO			50

// THREADS_META				//
#define DEFAULT_NB_THREADS          1
#define DEFAULT_SEED                0
#define DEFAULT_RLU_MAX_WS          1

// RESULTS_META             //
#define DEFAULT_LOG_OPT             0

workloads_meta_t work_meta;
results_meta_t results_meta;
index_meta_t index_meta;
threads_meta_t th_meta;

char *bench_set_file;
int log_option;

struct timeval start_time;
struct timeval end_time;
////////////////////////////////////////////////////////////////
// VARIABLES
////////////////////////////////////////////////////////////////
// BENCH MARK				//
struct timeval start, end;
struct timespec timeout;

static volatile long padding[50];

static volatile int stop;
static unsigned short main_seed[3];
static cpu_set_t cpu_set[450];

////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////
long elapsed_millis() {
    struct timeval now;
    gettimeofday(&now, NULL);

    long seconds = now.tv_sec - start_time.tv_sec;
    long useconds = now.tv_usec - start_time.tv_usec;

    return (seconds * 1000) + (useconds / 1000);
}

int op_make(int bench_type) {

    int r = rand() % 100 + 1;
    int stack = 0;
    
    if (bench_type == 1) {
        stack = work_meta.ratio_add;
        if (r <= stack) return 1;
        stack = stack + work_meta.ratio_remove;
        if (r <= stack) return 2;
        stack = stack + work_meta.ratio_search;
        if (r <= stack) return 3;
        stack = stack + work_meta.ratio_scan;
        if (r <= stack) return 4;
    }

    return 0;
}

int MarsagliaXORV(int x) {
    if (x == 0) x = 1;
    x ^= x << 6;
    x ^= ((unsigned)x) >> 21;
    x ^= x << 7;
    return x;
}

int MarsagliaXOR(int *seed) {
    int x = MarsagliaXORV(*seed);
    *seed = x;
    return x & 0x7FFFFFFF;
}

int srand_init(unsigned short seed) {
    if (seed == 0) {
        srand((int)time(NULL));
    } else {
        srand(seed);
    }
}

void rand_init(unsigned short *seed) {
    seed[0] = (unsigned)rand();
    seed[1] = (unsigned)rand();
    seed[2] = (unsigned)rand();
}

int rand_in_range(int min, int max, unsigned short *seed) {
    /* Return a random number in range [0;n) */
  
    /*int v = (int)(erand48(seed) * n);
    assert (v >= 0 && v < n);*/

    int v = min + (MarsagliaXOR((int *)seed) % (max - min + 1));

    return v;
}

void free_rand(unsigned short *seed) {
    free(seed);
    seed = NULL;
}

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

////////////////////////////////////////////////////////////////
// BENCHMARK FUNCTIONS
////////////////////////////////////////////////////////////////
void options_meta_set(int argc, char **argv) {
    int c, idx;

    struct option long_options[] = {
        {"duration",                        required_argument, NULL, 0},
        {"key_range",                       required_argument, NULL, 0},
        {"initial_size",                    required_argument, NULL, 0},
        {"zipf_dist_val",                   required_argument, NULL, 0},
        {"add_ratio",                       required_argument, NULL, 0},
        {"remove_ratio",                    required_argument, NULL, 0},
        {"search_ratio",                    required_argument, NULL, 0},
        {"scan_ratio",                      required_argument, NULL, 0},
        {"inner_degree",                    required_argument, NULL, 0},
        {"leaf_degree",                     required_argument, NULL, 0},
        {"split_threshold_ratio",           required_argument, NULL, 0},
        {"merge_threshold_ratio",           required_argument, NULL, 0},
        {"distribution_ratio",              required_argument, NULL, 0},
        {"num_threads",                     required_argument, NULL, 0},
        {"rlu_max_ws",                      required_argument, NULL, 0},
        {"seed",                            required_argument, NULL, 0},
        {NULL, 0, NULL, 0}
    };

    // DEFALUT SET
    work_meta.duration = DEFAULT_DURATION;
    work_meta.key_range = DEFAULT_KEY_RANGE;
    work_meta.init_size = DEFAULT_INIT_SIZE;
    work_meta.zipf_dist_val = DEFAULT_ZIPF_DIST_VAL;
	work_meta.ratio_add = DEFAULT_ADD_RATIO;
	work_meta.ratio_remove = DEFAULT_REMOVE_RATIO;
	work_meta.ratio_search = DEFAULT_SEARCH_RATIO;
	work_meta.ratio_scan= DEFAULT_SCAN_RATIO;
    work_meta.seed = DEFAULT_SEED;

    index_meta.inner_node_degree = DEFAULT_INNER_DEGREE;
	index_meta.leaf_node_degree = DEFAULT_LEAF_DEGREE;
	index_meta.split_thres_r = DEFAULT_SPLIT_THRESHOLD_RATIO;
	index_meta.merge_thres_r = DEFAULT_MERGE_THRESHOLD_RATIO;
	index_meta.distri_r = DEFAULT_DISTRIBUTION_RATIO;

    th_meta.nb_threads = DEFAULT_NB_THREADS;
	th_meta.rlu_max_ws = DEFAULT_RLU_MAX_WS;

    results_meta.log_opt = DEFAULT_LOG_OPT;

    while ((c = getopt_long(argc, argv, "", long_options, &idx) != -1)) {

            const char *name = long_options[idx].name;
            const char *arg = optarg ? optarg : "";

            if(strcmp(name, "duration") == 0)                       work_meta.duration = atoi(arg);
            else if(strcmp(name, "key_range") == 0)                 work_meta.key_range = atoi(arg);
            else if (strcmp(name, "initial_size") == 0)             work_meta.init_size = atoi(arg);
            else if (strcmp(name, "zipf") == 0)                     work_meta.zipf = atoi(arg);
            else if (strcmp(name, "zipf_dist_val") == 0)            work_meta.zipf_dist_val = strtod(arg, NULL);
            else if (strcmp(name, "add_ratio") == 0)                work_meta.ratio_add = atoi(arg);
            else if (strcmp(name, "remove_ratio") == 0)             work_meta.ratio_remove = atoi(arg);
            else if (strcmp(name, "search_ratio") == 0)             work_meta.ratio_search = atoi(arg);
            else if (strcmp(name, "scan_ratio") == 0)               work_meta.ratio_scan = atoi(arg);
            else if (strcmp(name, "inner_degree") == 0)             index_meta.inner_node_degree = atoi(arg);
            else if (strcmp(name, "leaf_degree") == 0)              index_meta.leaf_node_degree = atoi(arg);
            else if (strcmp(name, "split_threshold_ratio") == 0)    index_meta.split_thres_r = atoi(arg);
            else if (strcmp(name, "merge_threshold_ratio") == 0)    index_meta.merge_thres_r = atoi(arg);
            else if (strcmp(name, "distribution_ratio") == 0)       index_meta.distri_r = atoi(arg);
            else if (strcmp(name, "num_threads") == 0)              th_meta.nb_threads = atoi(arg);
            else if (strcmp(name, "rlu_max_ws") == 0)               th_meta.rlu_max_ws = atoi(arg);
            else if (strcmp(name, "seed") == 0)                     work_meta.seed = atoi(arg);
    }

    if (work_meta.zipf_dist_val > 0) { work_meta.zipf = 1;}

    assert(work_meta.duration >= 0);
    assert(work_meta.key_range < work_meta.init_size || work_meta.key_range > MAX_KEY);
    
    assert(index_meta.inner_node_degree >= 3 && index_meta.inner_node_degree <= MAX_INNER_DEGREE);
    assert(index_meta.leaf_node_degree >= 3 && index_meta.leaf_node_degree <= MAX_LEAF_DEGREE);
    assert(0 < index_meta.split_thres_r && index_meta.split_thres_r <=100);
    assert(0 < index_meta.merge_thres_r && index_meta.merge_thres_r <= 100);
    assert(0 < index_meta.distri_r && index_meta.distri_r <= 100);
    assert(work_meta.ratio_add + work_meta.ratio_remove + work_meta.ratio_search + work_meta.ratio_scan == 100);
    
    assert(th_meta.nb_threads > 0);
    assert(th_meta.rlu_max_ws >= 1 && th_meta.rlu_max_ws <= 100);

}

void options_meta_print() {
    printf("----------          WORK LOAD       ----------\n");
    printf("Duration                : %d Milliseconds\n", work_meta.duration);
    printf("Key range               : %d\n", work_meta.key_range);
    printf("Init size               : %d\n", work_meta.init_size);
    printf("Zipf val                : %lf\n", work_meta.zipf_dist_val);
    printf("Ratio of add            : %d\n", work_meta.ratio_add);
    printf("Ratio of remove         : %d\n", work_meta.ratio_remove);
    printf("Ratio of search         : %d\n", work_meta.ratio_search);
    printf("Ratio of scan           : %d\n", work_meta.ratio_scan);
    printf("----------    INDEX META DATA     ----------\n");
    printf("Inner node degree       : %d\n", index_meta.inner_node_degree);
    printf("Leaf node degree        : %d\n", index_meta.leaf_node_degree);
    printf("Split threshold ratio   : %d\n", index_meta.split_thres_r);
    printf("Merge threshold ratio   : %d\n", index_meta.merge_thres_r);
    printf("Distribution ratio      : %d\n", index_meta.distri_r);
    printf("----------   THREADS META DATA      ----------\n");
    printf("Nb threads              : %d\n", th_meta.nb_threads);
    printf("Rlu-max-ws              : %d\n", th_meta.rlu_max_ws);
    printf("seed                    : %d\n", work_meta.seed);
    printf("----------  ITEM TYPE METADATA      ----------\n");
    printf("MIN ~ MAX_KEY           : INT (%d ~ %d)\n", MIN_KEY, MAX_KEY);
    
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
    printf("Thread type sizes       :   rlu=%d\n",
                                        (int)sizeof(multi_thread_data_t));
    printf("\n\n");
}

void resluts_print() {
    printf("----------      SET BY RUNNING-TIME         ----------\n");
	printf("Given   Duration    : %d Milliseconds\n", work_meta.duration);
    printf("Actual  Duration    : %d Milliseconds\n", results_meta.duration);
    printf("\n\n");
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
            printf(" SEARCH ");
            break;
        case 4:
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

    if (so_log.operator < 4) {
        printf(" KEY : %d\n", so_log.key);
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

    if (option < 2) {return 1;}

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

void local_rlu_thread_print(multi_thread_data_t *p_data, int option) {

    for (int i = 0; i < th_meta.nb_threads; i++) {
        // printf("\n---------- THREAD %ld ----------\n", p_data[i].uniq_id);
        // printf("The number of ADD               : %d\n", p_data[i].nb_add);
        // printf("The number of REMOVE            : %d\n", p_data[i].nb_remove);
        // printf("The number of SEARCH            : %d\n", p_data[i].nb_search);
        // printf("The number of SCAN              : %u\n", p_data[i].nb_scan);
        // printf("Modified size by Thread         : %d\n", p_data[i].diff);

        results_meta.nb_add += p_data[i].nb_add;
        results_meta.nb_remove += p_data[i].nb_remove;
        results_meta.nb_search += p_data[i].nb_search;
        results_meta.nb_scan += p_data[i].nb_scan;
        results_meta.diff += p_data[i].diff;
    }

    results_meta.size = work_meta.init_size + results_meta.nb_add - results_meta.nb_remove;

    printf("\n---------- TOTAL THREAD ----------\n");
    printf("Total ADD               : %lu\n", results_meta.nb_add);
    printf("Total REMOVE            : %lu\n", results_meta.nb_remove);
    printf("Total SEARCH            : %lu\n", results_meta.nb_search);
    printf("Total SCAN              : %lu\n", results_meta.nb_scan);
    printf("Total Modified Size     : %ld\n", results_meta.diff);
    printf("Index Size              : %lu\n", results_meta.size);
}

void local_rlu_thread_log_print(multi_thread_data_t *p_data, int option) {
    if (!p_data) {
        perror("Invalid parameter for pure_so_print");
        exit(1);
    }

    if (!results_meta.log_opt) { return; }

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

////////////////////////////////////////////////////////////////
// THREADS FUNCTIONS
////////////////////////////////////////////////////////////////
pthread_t *local_new_rlu_thread() {
    pthread_t *p_rlu_threads;

    p_rlu_threads = (pthread_t *)malloc(th_meta.nb_threads * sizeof(pthread_t));

    if(p_rlu_threads == NULL) {
        perror("rlu threads pthread malloc");
        exit(1);
    }

    return p_rlu_threads;
}

multi_thread_data_t *local_new_rlu_data() {
    multi_thread_data_t *p_rlu_data;
    p_rlu_data = (multi_thread_data_t *)malloc(th_meta.nb_threads * sizeof(multi_thread_data_t));

    if(p_rlu_data == NULL) {
        perror("rlu thread data malloc");
        exit(1);
    }

    memset(p_rlu_data, 0, th_meta.nb_threads * sizeof(multi_thread_data_t));
    
    return p_rlu_data;
}

multi_thread_log_t *local_new_rlu_thread_log() {
    multi_thread_log_t *p_multi_thread_log;

    p_multi_thread_log = (multi_thread_log_t *)malloc(10000 * sizeof(multi_thread_log_t));
    memset(p_multi_thread_log, 0, 10000 * sizeof(multi_thread_log_t));

    if (!p_multi_thread_log) {
        perror("Failed to allocate log for local_new_single_thread_log");
        exit(1);
    }
    
    return p_multi_thread_log;
}

void local_rlu_thread_init(multi_thread_data_t *p_data) {
    for(int i = 0; i < th_meta.nb_threads; i++){
        p_data[i].uniq_id = i;
#ifdef IS_MVRLU
            p_data[i].p_rlu_td = RLU_THREAD_ALLOC();
#else
            p_data[i].p_rlu_td = &(p_data[i].rlu_td);
        #endif
        RLU_THREAD_INIT(p_data[i].p_rlu_td);   
    }
    return;
}

void local_rlu_thread_set(pthread_t *p_thread, multi_thread_data_t *p_data, barrier_t *p_barrier, pthread_attr_t *p_attr, root_node_t* p_root) {
    if(p_data == NULL) {
        perror("service thread is empty");
        exit(1);
    }

    for (int i = 0; i < th_meta.nb_threads; i++) {
        rand_init(p_data[i].seed);
        p_data[i].p_barrier = p_barrier;
        p_data[i].p_root = p_root;
        if (results_meta.log_opt) {p_data[i].p_log = local_new_rlu_thread_log();}
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

void local_rlu_thread_finish(multi_thread_data_t *p_data){
    for(int i = 0; i < th_meta.nb_threads; i++){
        RLU_THREAD_FINISH(p_data[i].p_rlu_td);
    }
}

// LOCAL
void local_new_thread(pthread_t **pp_rlu_thread, multi_thread_data_t **pp_rlu_data) {
#ifdef IS_RLU
    *pp_rlu_thread = local_new_rlu_thread();
    *pp_rlu_data = local_new_rlu_data();
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif //IS_RLU

}

void local_thread_init(multi_thread_data_t *p_rlu_data) {

#ifdef IS_RLU
    local_rlu_thread_init(p_rlu_data);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif //IS_RLU
    return;
}

void local_thread_set(pthread_t *p_rlu_thread, multi_thread_data_t *p_rlu_data,
                        barrier_t *p_barrier, pthread_attr_t *p_attr, root_node_t* p_root) {
#ifdef IS_RLU
    local_rlu_thread_set(p_rlu_thread, p_rlu_data, p_barrier, p_attr, p_root);
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif  //IS_RLU
}

void local_thread_wait(pthread_t *p_rlu_thread) {
#ifdef IS_RLU
    local_rlu_thread_wait(p_rlu_thread);
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif  //IS_RLU
}
    
void local_thread_finish(multi_thread_data_t **pp_rlu_data) {
#ifdef IS_RLU
    local_rlu_thread_finish(*pp_rlu_data);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif //IS_RLU
}

void local_free_thread(pthread_t **pp_rlu_thread, multi_thread_data_t **pp_rlu_data) {
#ifdef IS_RLU
    free(*pp_rlu_data);
    free(*pp_rlu_thread);
    *pp_rlu_data = NULL;
    *pp_rlu_thread = NULL;
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif //IS_RLU
}

// GLOBAL
void global_thread_init() {

#ifdef IS_RLU
    RLU_INIT();
#else
    printf("ERROR: benchmark not defined!\n");
    abort();
#endif  //IS_RLU
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
// SERVICE OPERATOR
/////////////////////////////////////////////////////////
int lb_tree_add(multi_thread_data_t *p_rlu_data, int input, int option) {
    int result;
#ifdef IS_RLU
    result = rlu_lb_tree_add(p_rlu_data, input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
    return result;
}

int lb_tree_remove(multi_thread_data_t *p_rlu_data, int input, int option) {
    int result;
#ifdef IS_RLU
    result = rlu_lb_tree_remove(p_rlu_data, input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
    return result;
}

int lb_tree_search(multi_thread_data_t *p_rlu_data, int input, int option) {
    int result;

#ifdef IS_RLU
    result = rlu_lb_tree_search(p_rlu_data, input, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU

    return result;
}

int lb_tree_range_scan(multi_thread_data_t *p_rlu_data, int start_key, int band_width, int option) {
    int result;
#ifdef IS_RLU
    result = rlu_lb_tree_range_scan(p_rlu_data, start_key, band_width, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
    return result;
}

/////////////////////////////////////////////////////////
// SPLIT MERGE OPERATION
/////////////////////////////////////////////////////////
int lb_tree_split(multi_thread_data_t *p_rlu_data, int option) {
   int result;

#ifdef IS_RLU
   result = rlu_lb_tree_split(p_rlu_data, option);
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
   return result;
}

int lb_tree_merge(multi_thread_data_t *p_rlu_data,  int option) {
    int result;
 #ifdef IS_RLU
    result = rlu_lb_tree_merge(p_rlu_data, option);
 #else
     printf("ERROR: benchmark not defined!\n");
     abort();
 #endif  // IS_RLU
    return result;
 }

/////////////////////////////////////////////////////////
// INIT AND SET
/////////////////////////////////////////////////////////
root_node_t* new_lbtree() {
    root_node_t *p_new;
#ifdef IS_RLU
    p_new = rlu_new_root();
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
    return p_new;
}

void new_smo(multi_thread_data_t *p_rlu_data) {
#ifdef IS_RLU
    for(int i = 0; i < th_meta.nb_threads; i++) {
        rlu_new_smo(&p_rlu_data[i]);
    }
#else
	printf("ERROR: benchmark not defined!\n");
	abort();
#endif  // IS_RLU
    return;
}

int lb_tree_init(multi_thread_data_t *p_data) {
    int init_size, temp, op_result, key;

    init_size = 0;

    if (p_data->uniq_id == 0) {
        memset(p_data->p_smo, 0, sizeof(smo_t));
        printf("[%ld] Initializing\n", p_data->uniq_id);
        printf("[%ld] Adding %d entries to set\n", p_data->uniq_id, work_meta.init_size);
    } else { return -1;}

#if defined(IS_RLU) && defined(IS_LBTREE)
    while (true) {
        if (p_data->p_smo->operator == 0) {
            key = rand_in_range(MIN_KEY, work_meta.key_range, p_data->seed) + 1;
            if (lb_tree_add(p_data, key, 2) == SUCCESS) {init_size ++;}
        } else {
            temp = p_data->p_smo->operator;
            op_result = rlu_lb_tree_split(p_data, 2);
        }

        if (temp == p_data->p_smo->operator) {
            memset(p_data->p_smo, 0, sizeof(smo_t));
            temp = 0;
            if (init_size < work_meta.init_size) {break;}
        }
    }
#else
#endif
    printf("[%ld] Adding done\n", p_data->uniq_id);
}

/////////////////////////////////////////////////////////
// PRINT OPERATION
/////////////////////////////////////////////////////////
void local_thread_print(multi_thread_data_t *p_rlu, int option) {
    printf("---------- BENCHMARK RESULTS ----------\n");
    printf("Execution Time          : %d msec", results_meta.duration);
    #ifdef IS_RLU
        // local_rlu_thread_log_print (p_rlu, option);
        local_rlu_thread_print(p_rlu, option);
    #else
        printf("ERROR: benchmark not defined!\n");
        abort();
    #endif  //IS_RLU
    printf("\n\n");
}
    
void lb_tree_print(multi_thread_data_t *p_rlu_data, int start_lv, int end_lv, int option) {
#ifdef IS_RLU
    rlu_lb_tree_print(p_rlu_data, start_lv, end_lv, option);
#endif // IS_RLU
    return;
}

/////////////////////////////////////////////////////////
// TEST_THREADS
/////////////////////////////////////////////////////////

void* rlu_test(void *arg) { 
    multi_thread_data_t* p_data = (multi_thread_data_t *)arg;
    int key, update_key, basic_op_result, sm_op_result, basic_command, start, end, temp;

    struct zipf_state zs;

    zipf_init(&zs, work_meta.key_range, work_meta.zipf, rand_in_range(MIN_KEY, work_meta.key_range, p_data->seed)+1);

// ===== 측정 대상 코드 시작 =====
    lb_tree_init(p_data);
#ifdef IS_MVRLU
    mvrlu_flush_log(p_data->p_rlu_td);
#endif

    barrier_cross(p_data->p_barrier);
    gettimeofday(&start_time, NULL);

    while(stop == 0) {
         
        basic_command = op_make(1);

        basic_op_result = 0;
        sm_op_result = 0;

        if (p_data->zipf) {
            key = zipf_next(&zs);
        } else {
            key = rand_in_range(MIN_KEY, work_meta.key_range, p_data->seed)+1;
        }

        if (p_data->p_smo->operator == 0) {
            switch(basic_command) {
                case 1:
                    basic_op_result = lb_tree_add(p_data, key, 2);
                    if (basic_op_result > 0) {
                        p_data->nb_add++;
                        p_data->diff++;
                    }
                    break;
                case 2:
                    basic_op_result = lb_tree_remove(p_data, key, 2);
                    if (basic_op_result > 0) {
                        p_data->nb_remove++;
                        p_data->diff--;
                    }
                    break;
                case 3:
                   basic_op_result = lb_tree_search(p_data, key, 2);
                   if (basic_op_result > 0) {
                       p_data->nb_search++;
                   }
                   break;
                case 4:
                    basic_op_result = lb_tree_range_scan(p_data, key, 100, 2);
                    break;
            }
        } else {
            temp = p_data->p_smo->operator;
            if (temp > 0) {
                sm_op_result = lb_tree_split(p_data, 2);
                if (sm_op_result) {
                    if (temp == 10) {p_data->nb_header_split++;}
                    else {p_data->nb_inner_split++;}
                }
            } else {
                sm_op_result = lb_tree_merge(p_data, 2);
                if (sm_op_result) {
                    if (temp == -10) {p_data->nb_header_merge++;}
                    else {p_data->nb_inner_merge++;}
                }
            }
            if (temp == p_data->p_smo->operator) {
                memset(p_data->p_smo, 0, sizeof(smo_t));
            }
        }
    }
    return NULL;
}

/////////////////////////////////////////////////////////
// SIGNAL HANDLING
/////////////////////////////////////////////////////////
void trigger_print() {
    // results_print();
        
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

    pthread_t *p_rlu_thread;
    multi_thread_data_t *p_rlu_data;

    root_node_t *p_root;

    local_new_thread(&p_rlu_thread, &p_rlu_data);
    
    p_root = new_lbtree();

    new_smo(p_rlu_data);

    sa.sa_handler = segfault_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    signal(SIGINT, forced_termination_handler);

    timeout.tv_sec = work_meta.duration / 1000;
    timeout.tv_nsec = (work_meta.duration % 1000) * 1000000;

    barrier_init(&barrier, th_meta.nb_threads+1);
    pthread_attr_init(&attr);

    srand_init(work_meta.seed);
    rand_init(main_seed);
    global_thread_init();
    local_thread_init(p_rlu_data);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    local_thread_set(p_rlu_thread, p_rlu_data, &barrier, &attr, p_root);

    pthread_attr_destroy(&attr);

    stop = 0;
    barrier_cross(&barrier);

    printf("\n\n==========      STARTING THREADS        ==========\n\n");

    gettimeofday(&start, NULL);
    if (work_meta.duration > 0) {
        nanosleep(&timeout, NULL);
        stop = 1;
    }
    
    local_thread_wait(p_rlu_thread);
    gettimeofday(&end, NULL);
    printf("\n\n==========      STOPPING THREADS        ==========\n\n");


    results_meta.duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
    
    local_thread_finish(&p_rlu_data);

    printf("\n\n==========      PRINT MEASUREMENTS        ==========\n\n"); 
    
    local_thread_print(p_rlu_data, 2);

    // lb_tree_print(p_rlu_data, 10, -1, 2);

    local_free_thread(&p_rlu_thread, &p_rlu_data);
    free_rand(p_thread_seed);

    print_thread_stats();

    printf("\n\n==========      FINISH BENCHMARK        ==========\n\n");
	return 0;
}
