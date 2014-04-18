#include "inter_code_gen.h"

// Ptrs to instructions. Indirect-triplet approach
// Triple list should NEVER be reorderd. As we reference places in it from _intr_list.

GPtrArray *init_instr_list()
{
        return g_ptr_array_new();
}

void add_instr(GPtrArray *instr_list, int *num_instrs, Instruction *instr)
{
    *num_instrs += 1;
    g_ptr_array_add(instr_list, (gpointer) instr);
}

char *get_arg_repr(Arg arg)
{
    char *repr;
    switch (arg.type)
    {
        case CONST:
            repr = "CONSTANT";
            break;
        case INSTR:
            repr = "INSTR";
            break;
        case IDENT:
            repr = "IDENT";
            break;
        default:
            repr = "UNKOWN";
            break;
    }
    return repr;
}

void print_instr_list(GPtrArray *instr_list, int num_instrs)
{
    Instruction *instr;
    //for (int i=0; i < num_instrs; i++)
    for (int i=0; i < 1; i++)
    {
        instr = (Instruction*) g_ptr_array_index(instr_list, i);
        printf("%d:%s:%s\n", instr->op_code, 
                get_arg_repr(instr->arg1),get_arg_repr(instr->arg2));
    }
}
