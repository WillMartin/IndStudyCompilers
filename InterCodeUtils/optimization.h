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
    // Match the instr_list
    // Up to but not including e_idx
    int s_idx;
    int e_idx;
} BasicBlock;

GList *make_blocks(GPtrArray *instr_list, int num_instrs);
void print_blocks(GPtrArray *instr_list, int num_instrs, GList *block_list);

#endif  // OPTIMIZATION
