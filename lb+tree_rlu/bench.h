#ifndef BENCH_H_
#define BENCH_H_
////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////

#include "lb_tree.h"
////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////
// WORK LOADS				//
#define DEFAULT_DURATION			10000
#define DEFAULT_CYCLE				200

// WORK LOADS DISTRI		//
#define DEFAULT_ADD_RATIO			30
#define DEFAULT_REMOVE_RATIO		10
#define DEFAULT_UPDATE_RATIO	    10
#define DEFAULT_SEARCH_RATIO		45
#define DEFAULT_SCAN_RATIO			5

// HB+TREE_COND				//
#define DEFAULT_TREE_DOCK_DEGREE  		    4
#define DEFAULT_NODE_DOCK_DEGREE			10
#define DEFAULT_SPLIT_THRESHOLD_RATIO		100
#define DEFAULT_MERGE_THRESHOLD_RATIO		25
#define DEFAULT_DISTRIBUTION_RATIO			50
#define DEFAULT_LOAD_TREE					-1

// THREADS_META				//
#define DEFAULT_NB_THREADS          1
#define DEFAULT_BASE_SEED           0
#define DEFAULT_RLU_MAX_WS          1

////////////////////////////////////////////////////////////////
// VARIABLES
////////////////////////////////////////////////////////////////
// BENCH MARK				//
static volatile long padding[50];

static volatile int stop;
static volatile atomic_int global_cycle;
static unsigned short base_seed;

static single_thread_data_t *g_p_single;
static rlu_multi_thread_data_t *g_p_rlu;
static root_node_t *g_p_root;

////////////////////////////////////////////////////////////////
// INTERFACE
////////////////////////////////////////////////////////////////

#endif