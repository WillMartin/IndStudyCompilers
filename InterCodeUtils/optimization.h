#ifndef OPTIMIZATION
#define OPTIMIZATION

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "symbol_table.h"
#include "inter_code_gen.h"

typedef struct BasicBlock
{
    // Of Instruction*
    GList *instrs;
    // Of BasicBlock*
    GList *successors;
} BasicBlock;

GList *make_blocks(GPtrArray *instr_list, int num_instrs);
void print_blocks(GPtrArray *instr_list, int num_instrs, GList *block_list);
void combine_blocks(GList *block_list, GPtrArray **instr_list, int *num_values);



typedef struct DagNode
{
    eOPCode op_code;
} DagNode;

#endif  // OPTIMIZATION
