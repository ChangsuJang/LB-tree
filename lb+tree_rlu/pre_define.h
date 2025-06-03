#ifndef PRE_DEFINE_H_
#define PRE_DEFINE_H_

////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "rlu.h"
#include "operation_meta.h"

/////////////////////////////////////////////////////////
// DEFINES
/////////////////////////////////////////////////////////
//	SYSTEM						//
#define RO                              1
#define RW                              0

#ifdef DEBUG
# define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif

// VALUE						//

////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////

// ABSTRACTION					//
typedef struct root_node root_node_t;
typedef struct smo smo_t;
typedef struct thread_log thread_log_t;

// HELPER						//
typedef struct barrier {
  pthread_cond_t complete;
  pthread_mutex_t mutex;
  int count;
  int crossing;
} barrier_t;

// LOG
typedef struct so_log {
    int operator;
	int header_key[3];
	int header_state[3];
	int header_count[3];
	int start_key, end_key;
	item_t input;
	item_t update_input;
    int result;
} so_log_t;

typedef struct smo_log {
    int operator, result;
    int m_inner_key[3], s_inner_key[3], m_header_key[3], s_header_key[3];
} smo_log_t;

typedef struct single_thread_log {
	so_log_t so_log;
	smo_log_t smo_log;
}single_thread_log_t;

typedef struct rlu_multi_thread_log {
	int start_g_cycle;
	so_log_t so_log;
	smo_log_t smo_log;
	int end_g_cycle;
	int working;
}rlu_multi_thread_log_t;


// OPTIONS	META				//
typedef struct hardware_meta {
	int dram_s;
}hardware_meta_t;

typedef struct workloads_meta {
	int duration;
	int cycle;
	int ratio_add;
	int ratio_remove;
	int ratio_update;
	int ratio_search;
	int ratio_scan;
}workloads_meta_t;

typedef struct measurements_meta {
	int duration;
	int cycle;
}measurements_meta_t;

typedef struct lb_tree_meta {
	int inner_node_degree;
	int leaf_node_degree;
	int split_thres_r;
	int merge_thres_r;
	int distri_r;
	int load_tree;
}lb_tree_meta_t;

typedef struct threads_meta {
	int nb_threads;
	int rlu_max_ws;
	unsigned short base_seed;
}threads_meta_t;

// THREADS 						//
typedef struct single_thread_data {
	unsigned short seed;
	struct barrier *p_barrier;

	int nb_try_add;
	int nb_add;
	int nb_try_remove;
	int nb_remove;
	int nb_try_update;
	int nb_update;
	int nb_try_search;
	int nb_search;
	int nb_try_scan;
	int nb_scan;
	int nb_try_header_split;
	int nb_header_split;
	int nb_try_header_merge;
	int nb_header_merge;
	int nb_try_inner_split;
	int nb_inner_split;
	int nb_try_inner_merge;
	int nb_inner_merge;
	int diff;
	int local_cycle;

	root_node_t *p_root;
	smo_t *p_smo;
	
	int log_option;
	single_thread_log_t *p_log;
} single_thread_data_t;

typedef struct rlu_multi_thread_data {
	long uniq_id;
	unsigned short seed;
	struct barrier *p_barrier;

	int nb_try_add;
	int nb_add;
	int nb_try_remove;
	int nb_remove;
	int nb_try_update;
	int nb_update;
	int nb_try_search;
	int nb_search;
	int nb_try_scan;
	int nb_scan;
	int nb_try_header_split;
	int nb_header_split;
	int nb_try_header_merge;
	int nb_header_merge;
	int nb_try_inner_split;
	int nb_inner_split;
	int nb_try_inner_merge;
	int nb_inner_merge;
	int diff;
	int local_cycle;

	root_node_t *p_root;
	smo_t *p_smo;

	rlu_thread_data_t *p_rlu_td;
	rlu_thread_data_t rlu_td;

	int log_option;
	rlu_multi_thread_log_t *p_log;

} rlu_multi_thread_data_t;

////////////////////////////////////////////////////////////////
// ABSTRACTION
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// VARIABLES
/////////////////////////////////////////////////////////
// OPTIONS META				//
extern hardware_meta_t hard_meta;
extern workloads_meta_t work_meta;
extern measurements_meta_t meas_meta;
extern lb_tree_meta_t lb_meta;
extern threads_meta_t th_meta;

extern char *bench_set_file;
extern char *load_tree_file;
extern int log_option;
/////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////
// HELPER					//
void new_rand(unsigned short src_seed, unsigned short **obj_seed);
void free_rand(unsigned short *seed);


// OPERATION GENERATOR		//
int op_make();
item_t item_make();
int key_compare(int src, int obj);
void bench_set_file_read(const char *filename, int *argc, char ***argv);

// THREADS					//
void barrier_init(barrier_t *b, int n);
void barrier_cross(barrier_t *b);

pthread_t *local_new_single_thread();
single_thread_data_t *local_new_single_data();
single_thread_log_t *local_new_single_thread_log();
void local_single_thread_set(pthread_t *p_thread, single_thread_data_t *p_data, barrier_t *p_barrier, pthread_attr_t *p_attr, unsigned short *p_seed, root_node_t *p_root);
void local_single_thread_wait(pthread_t *p_thread);

pthread_t *local_new_rlu_thread();
rlu_multi_thread_data_t *local_new_rlu_data();
rlu_multi_thread_log_t *local_new_rlu_thread_log();
void local_rlu_thread_init(rlu_multi_thread_data_t *p_data);
void local_rlu_thread_set(pthread_t* p_thread, rlu_multi_thread_data_t *p_data, barrier_t *p_barrier, pthread_attr_t *p_attr, unsigned short *p_seed, root_node_t* p_root);
void local_rlu_thread_wait(pthread_t *p_thread);

void local_rlu_thread_finish(rlu_multi_thread_data_t *p_data);

// PRINT					//
void item_print(item_t src);
int so_log_print(so_log_t so_log, int option);
int smo_log_print(smo_log_t smo_log, int option);
void local_single_thread_print(single_thread_data_t *p_data, int option);
void local_single_thread_log_print (single_thread_data_t *p_data, int option);
void local_rlu_thread_print(rlu_multi_thread_data_t *p_data, int option);
void local_rlu_thread_log_print (rlu_multi_thread_data_t *p_data, int option);

// TEST					//
void *single_test(void *arg);
void *rlu_test(void *arg);


#endif