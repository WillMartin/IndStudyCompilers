#include "optimization.h"


void print_blocks(GPtrArray *instr_list, int num_instrs, GList *block_list)
{
    for (; block_list!=NULL; block_list=block_list->next)
    {
        BasicBlock *block = block_list->data;
        GList *instr = block->instrs;
        printf("------------START BASIC BLOCK ------------\n");
        for (; instr!=NULL; instr=instr->next)
        {
            print_instr((Instruction*)instr->data); 
        }
        printf("------------------------------------------\n");
    }
}


void combine_blocks(GList *block_list, GPtrArray **instr_list, int *num_values)
{
    GList *head = block_list;
    *num_values = 0;
    *instr_list = g_ptr_array_new();
    for (; head!=NULL; head=head->next)
    {
        GList *instr = ((BasicBlock*)head->data)->instrs;
        for (; instr!=NULL; instr=instr->next)
        {
            g_ptr_array_add(*instr_list,(Instruction*)instr->data);
            *num_values += 1;
        }
    }
}

// Not super efficient but fine for now
BasicBlock *get_addr_block(GList *blocks, Instruction *to_find)
{
   GList *head = blocks;
   for (; head!=NULL; head=head->next)
    {
        BasicBlock *cur_block = head->data;
        GList *instr = cur_block->instrs;
        for (; instr!=NULL; instr=instr->next)
        {

            if (((Instruction*)instr->data) == to_find)
            {
                return cur_block;
            }
        }
    }
    // Should always be in a block
    assert(false);
    return NULL;
}

// Adds successor ptrs in place to the list of BasicBlocks
void add_successors(GList *blocks)
{
    GList *head = blocks;
    while (head!=NULL)
    {
        BasicBlock *cur_block = head->data;
        cur_block->successors = NULL;
        GList *cur_data = cur_block->instrs;
        for (; cur_data!=NULL; cur_data=cur_data->next)
        {
            Instruction *instr = cur_data->data;
            if (instr->goto_addr != NULL)
            {
                BasicBlock *new_succ = get_addr_block(blocks, instr->goto_addr);
                cur_block->successors = g_list_prepend(cur_block->successors, new_succ);
            }
        }
        head=head->next;
        if (head != NULL)
        {
            BasicBlock *next_block = head->data;
            cur_block->successors = g_list_prepend(cur_block->successors, next_block);
        }
    }
}

GList *make_blocks(GPtrArray *instr_list, int num_instrs)
{
    if (num_instrs == 0) { return NULL; }

    GList *block_list = NULL;
    // First 3-addr ins is a leader
    BasicBlock *cur_block = malloc(sizeof(BasicBlock)); 
    cur_block->instrs = NULL;
    //cur_block->successors= NULL;

    Instruction *cur_instr, *last_instr;
    last_instr = get_instr(instr_list, num_instrs, 0);
    cur_block->instrs = g_list_prepend(cur_block->instrs, last_instr);
    for (int i=1; i<num_instrs; i++)
    {
        cur_instr = get_instr(instr_list, num_instrs, i);

        bool start_new_block = false;
        // Any inst with a label is a leader
        if (cur_instr->label != NULL) { start_new_block = true; }
        // any instr that immediately follows a jmp is a leader
        if (last_instr->op_code == GOTO || is_relative_op(last_instr->op_code))
        {
            start_new_block = true;
        }

        if (start_new_block)
        {
            cur_block->instrs = g_list_reverse(cur_block->instrs);
            //cur_block->successors = g_list_prepend(cur_block->successors, 
            // Prepending is more efficient
            block_list = g_list_prepend(block_list, cur_block);
            //BasicBlock *old_block = cur_block;
            cur_block = malloc(sizeof(BasicBlock));
            //old_block->successors = g_list_prepend(old_block->successors, cur_block);
            cur_block->instrs = NULL;
            cur_block->instrs = g_list_prepend(cur_block->instrs, cur_instr);
        }
        else
        {
            cur_block->instrs = g_list_prepend(cur_block->instrs, cur_instr);
        }

        last_instr = cur_instr;
    }
    cur_block->instrs = g_list_reverse(cur_block->instrs);
    block_list = g_list_prepend(block_list, cur_block);
    block_list = g_list_reverse(block_list);
    add_successors(block_list);

    return block_list;
}

