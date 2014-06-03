#ifndef OPTIMIZATION
#define OPTIMIZATION

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "symbol_table.h"
#include "inter_code_gen.h"
#include "gc.h"

typedef struct BasicBlock
{
    // Of Instruction*
    GList *instrs;
    // Of BasicBlock*
    GList *successors;
} BasicBlock;

typedef struct DagBlock
{
    // If something has to be first it'll have a label (Entry into block)
    char *init_label; 
    GList *root_nodes;
    // If there's a GOTO at the end, we want it to stay there, can't move it around
    Instruction *exit_instr;
} DagBlock;

GList *make_blocks(GPtrArray *instr_list, int num_instrs);
void print_blocks(int num_instrs, GList *block_list);
void print_block(BasicBlock *block);
void combine_blocks(GList *block_list, GPtrArray **instr_list, int *num_values);

typedef struct DagNode
{
    // If it is an id, it will have more info,
    // Otherwise it's just a constant and will be basically an empty shell
    bool is_id;
    eOPCode op_code;
    // If it's an initial node, give it an init_arg for its initial value.
    Arg *init_arg;
    // List of identifiers of which it has the last value
    GList *ids;
    // 3-addr codes can only have two children
    struct DagNode *left;
    struct DagNode *right;

    // For recompiling dag/printing
    bool visited;
} DagNode;

DagBlock *generate_dag(BasicBlock *block);
void print_dag(DagBlock *dag);
BasicBlock *compile_dag(BasicBlock *block, DagBlock *dag);
void optimize(GPtrArray *init_instrs, int num_instrs,
              GPtrArray **out_instrs, int *out_num_instrs);

#endif  // OPTIMIZATION
