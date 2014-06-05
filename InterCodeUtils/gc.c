/* Garbage collection code. Basically throws everything into a giant list and then frees it when gc_free() is called */
#include "inter_code_gen.h"
#include "optimization.h"
#include "gc.h"
GList *gc_list;

void gc_add(ePtrType type, void *ptr)
{
    GCPtr *gc_ptr = malloc(sizeof(GCPtr));
    gc_ptr->type = type;
    gc_ptr->ptr = ptr;
    gc_list = g_list_prepend(gc_list, gc_ptr);
}

void gc_init()
{
    gc_list = NULL;
}

void gc_free()
{
    GList *freed = gc_list;
     
    while (freed!=NULL)
    {
        GCPtr *gc_ptr = freed->data;
        switch (gc_ptr->type)
        {
            case ARG_TYPE:;
                Arg *arg = gc_ptr->ptr;
                g_list_free(arg->true_list);
                g_list_free(arg->false_list);
                free(gc_ptr->ptr);
                break;
            case IDENT_TYPE:;
                Identifier *id = gc_ptr->ptr;
                g_list_free(id->address_descriptor);
                free(id);
                break;
            case CHAR_TYPE:
                free(gc_ptr->ptr);
                break;
            case GLIST_TYPE:
                g_list_free(gc_ptr->ptr);
                break;
            case BASIC_BLOCK_TYPE:;
                BasicBlock *b = gc_ptr->ptr;
                g_list_free(b->successors);
                g_list_free(b->instrs);
                free(b);
                break;
            case DAG_BLOCK_TYPE:;
                DagBlock *db = gc_ptr->ptr; 
                g_list_free(db->root_nodes);
                free(db);
                break;
            case DAG_NODE_TYPE:;
                DagNode *node = gc_ptr->ptr;
                g_list_free(node->ids);
                free(node);
                break;
            case CONSTANT_TYPE:
                free(gc_ptr->ptr);
                break;
            case INSTR_TYPE:;
                Instruction *instr = gc_ptr->ptr;
                g_list_free(instr->actions);
                free(instr);
                break;
            case ACTION_TYPE:
                free(gc_ptr->ptr);
                break;
        }
        GList *tmp = freed;
        freed=freed->next;
        free(tmp->data);
    }
    g_list_free(gc_list);
}

void *gc_malloc(ePtrType type, int size)
{
    void *x = malloc(size);
    gc_add(type, x);
    return x;
}
