#ifndef GC 
#define GC

#include "symbol_table.h"
#include <glib.h>

extern GList *gc_list;

typedef enum ePtrType 
{
    ARG_TYPE,
    IDENT_TYPE,
    CHAR_TYPE,
    GLIST_TYPE,
    GPTR_ARRAY_TYPE,
    BASIC_BLOCK_TYPE,
    DAG_BLOCK_TYPE,
    DAG_NODE_TYPE,
    CONSTANT_TYPE,
    INSTR_TYPE,
    ACTION_TYPE,
} ePtrType;

typedef struct GCPtr
{
    ePtrType type;
    void *ptr;
} GCPtr;



void gc_init();
void gc_free();
void *gc_malloc(ePtrType type, int size);
void gc_add(ePtrType type, void *ptr);
#endif 
