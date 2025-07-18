#ifndef OPERATION_META_H
#define OPERATION_META_H


////////////////////////////////////////////////////////////////
// INCLUDES
////////////////////////////////////////////////////////////////
#include "pre_define.h"

////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////

#define ITEM_MIN_KEY    100
#define ITEM_MAX_KEY    1000000

#define ITEM_MAX_VALUE  1
#define ITEM_MIN_VALUE  1000

////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////

// ITEM                         //
typedef struct item {
    int key;
    int value;
} item_t;

typedef struct item item_t;

#endif  //OPERATION_META_H
