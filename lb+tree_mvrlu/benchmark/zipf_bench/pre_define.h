#ifndef PRE_DEFINE_H_
#define PRE_DEFINE_H_

#define _GNU_SOURCE
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

#ifdef IS_MVRLU
#include "mvrlu.h"
#else
#include "rlu.h"
#endif

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
#define MIN_KEY    (1)
#define MAX_KEY    (INT_MAX - 1)

#define HEADER_KEY      (MIN_KEY - 1)
#define TAIL_KEY        (MAX_KEY + 1)
////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////

// ABSTRACTION					//
typedef struct root_node root_node_t;
typedef struct smo smo_t;
typedef struct multi_thread_log multi_thread_log_t;

// HELPER						//
typedef struct barrier {
  pthread_cond_t complete;
  pthread_mutex_t mutex;
  int count;
  int crossing;
} barrier_t;
// OPTIONS	META				//

typedef struct workloads_meta {
	int duration;
	int range;
	int init_size;
	int zipf;
	double zipf_dist_val;
	int ratio_add;
	int ratio_remove;
	int ratio_search;
	int ratio_scan;
	unsigned short seed;
}workloads_meta_t;

typedef struct results_meta {
	int duration;
	int log_opt;
	unsigned long nb_add;
	unsigned long nb_remove;
	unsigned long nb_search;
	unsigned long nb_scan;

	unsigned long diff;
	unsigned long size;
}results_meta_t;

typedef struct index_meta {
	int inner_node_degree;
	int leaf_node_degree;
	int split_thres_r;
	int merge_thres_r;
	int distri_r;
}index_meta_t;

typedef struct threads_meta {
	int nb_threads;
	int rlu_max_ws;
}threads_meta_t;

// THREADS 						//
typedef struct multi_thread_data {
	long uniq_id;
	unsigned short seed[3];
	struct barrier *p_barrier;

	int nb_add;
	int nb_remove;
	int nb_search;
	int nb_scan;
	int nb_header_split;
	int nb_header_merge;
	int nb_inner_split;
	int nb_inner_merge;

	int diff;
	int zipf;
	double zipf_dist_val;

	root_node_t *p_root;
	smo_t *p_smo;

	rlu_thread_data_t *p_rlu_td;
#ifndef IS_MVRLU
    rlu_thread_data_t rlu_td;
#endif

	multi_thread_log_t *p_log;

} multi_thread_data_t;


// LOG
typedef struct so_log {
    int operator;
	int header_key[3];
	int header_state[3];
	int header_count[3];
	int start_key, end_key;
	int key;
    int result;
} so_log_t;

typedef struct smo_log {
    int operator, result;
    int m_inner_key[3], s_inner_key[3], m_header_key[3], s_header_key[3];
} smo_log_t;

typedef struct multi_thread_log {
	int start_g_cycle;
	so_log_t so_log;
	smo_log_t smo_log;
	int end_g_cycle;
	int working;
}multi_thread_log_t;


////////////////////////////////////////////////////////////////
// ABSTRACTION
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// VARIABLES
/////////////////////////////////////////////////////////
// OPTIONS META				//
extern workloads_meta_t work_meta;
extern results_meta_t results_meta;
extern index_meta_t index_meta;
extern threads_meta_t th_meta;

extern char *bench_set_file;
extern int log_option;

extern struct timeval start_time;
extern struct timeval end_time;
/////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////
// HELPER					//
void new_rand(unsigned short src_seed, unsigned short **obj_seed);
void free_rand(unsigned short *seed);
int srand_init(unsigned short seed);
void rand_init(unsigned short *seed);

// OPERATION GENERATOR		//
int op_make(int bench_type);
int key_compare(int src, int obj);
void bench_set_file_read(const char *filename, int *argc, char ***argv);

// THREADS					//
void barrier_init(barrier_t *b, int n);
void barrier_cross(barrier_t *b);

pthread_t *local_new_rlu_thread();
multi_thread_data_t *local_new_rlu_data();
multi_thread_log_t *local_new__thread_log();
void local_rlu_thread_init(multi_thread_data_t *p_data);
void local_rlu_thread_set(pthread_t* p_thread, multi_thread_data_t *p_data, barrier_t *p_barrier, pthread_attr_t *p_attr, root_node_t* p_root);
void local_rlu_thread_wait(pthread_t *p_thread);

void local_rlu_thread_finish(multi_thread_data_t *p_data);

// PRINT					//
int so_log_print(so_log_t so_log, int option);
int smo_log_print(smo_log_t smo_log, int option);
void local_rlu_thread_print(multi_thread_data_t *p_data, int option);
void local_rlu_thread_log_print (multi_thread_data_t *p_data, int option);

// TEST					//
void *rlu_test(void *arg);


#endif