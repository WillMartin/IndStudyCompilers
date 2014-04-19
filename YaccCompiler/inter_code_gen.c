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

char *get_constant_repr(Constant c)
{
    char *repr;
    switch (c.type)
    {
        case INTEGER:
            repr = malloc(sizeof(char));
            sprintf(repr, "%d", c.int_val);
            break;
        case DOUBLE:
            repr = malloc(sizeof(char));
            sprintf(repr, "%f", c.float_val);
            break;
        case LONG:
            repr = malloc(sizeof(char));
            sprintf(repr, "%lu", c.long_val);
            break;
        case STRING:
            repr = c.str_val;
            break; 
        default:
            repr = "CONSTANT-ERROR";
            break;
    }
    return repr;
}

char *get_arg_repr(Arg arg)
{
    char *repr;
    switch (arg.type)
    {
        case CONST:
            repr = get_constant_repr(arg.const_val);
            break;
        case INSTR:
            repr = "INSTR";
            break;
        case IDENT:
            //repr = "IDENT";
            repr = arg.ident_val->symbol;
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

    for (int i=0; i < num_instrs; i++)
    {
        instr = (Instruction*) g_ptr_array_index(instr_list, i);
        printf("%d:%s:%s\n", instr->op_code, 
                get_arg_repr(instr->arg1),get_arg_repr(instr->arg2));
    }
}
