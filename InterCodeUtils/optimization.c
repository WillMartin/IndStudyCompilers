#include "optimization.h"


void print_blocks(GPtrArray *instr_list, int num_instrs, GList *block_list)
{
    for (; block_list!=NULL; block_list=block_list->next)
    {
        BasicBlock *block = block_list->data;
        printf("------------START BASIC BLOCK ------------\n");
        for (int i=block->s_idx; i<block->e_idx; i++)
        {
            Instruction *cur_instr = get_instr(instr_list, num_instrs, i);
            print_instr(cur_instr); 
        }
        printf("------------------------------------------\n");
    }
}

GList *make_blocks(GPtrArray *instr_list, int num_instrs)
{
    if (num_instrs == 0) { return NULL; }
// First 3-addr ins is a leader
// Any inst with a label
// any instr that immediately follows a jmp is a leader

// All blocks up to but not including next leader
    GList *block_list = NULL;
    BasicBlock *cur_block = malloc(sizeof(BasicBlock)); 
    cur_block->s_idx = 0;
    Instruction *cur_instr, *last_instr;
    last_instr = get_instr(instr_list, num_instrs, 0);
    for (int i=1; i<num_instrs; i++)
    {
        cur_instr = get_instr(instr_list, num_instrs, i);

        bool start_new_block = false;
        if (cur_instr->label != NULL) { start_new_block = true; }
        if (last_instr->op_code == GOTO || is_relative_op(last_instr->op_code))
        {
            start_new_block = true;
        }

        if (start_new_block)
        {
            cur_block->e_idx = i;
            block_list = g_list_prepend(block_list, cur_block);
            cur_block = malloc(sizeof(BasicBlock));
            cur_block->s_idx = i;
        }

        last_instr = cur_instr;
    }
    cur_block->e_idx = num_instrs;
    block_list = g_list_prepend(block_list, cur_block);
    block_list = g_list_reverse(block_list);
    return block_list;
}
