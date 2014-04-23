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


Instruction *get_instr(GPtrArray *instr_list, int num_instrs, int index)
{
    if (index >= num_instrs || index < 0) { return NULL; }

    return (Instruction *) g_ptr_array_index(instr_list, index);
}

char *get_constant_repr(Constant *c)
{
    // Little janky but it should work
    char *repr = malloc(50 * sizeof(char));
    switch (c->type)
    {
        case INTEGER:
            sprintf(repr, "%d", c->int_val);
            break;
        case DOUBLE:
            sprintf(repr, "%f", c->float_val);
            break;
        case LONG:
            sprintf(repr, "%lu", c->long_val);
            break;
        case CHAR:
            return c->str_val;
            break; 
        default:
            return "CONSTANT-ERROR";
            break;
    }

    return repr;
}

char *get_arg_repr(Arg *arg)
{
    char *repr = NULL;
    switch (arg->type)
    {
        case CONST:
            repr = get_constant_repr(arg->const_val);
            break;
        case INSTR:
            repr = "INSTR";
            break;
        case IDENT:
            repr = arg->ident_val->symbol;
            break;
        default:
            sprintf(repr, "UNKNOWN: %d", arg->type);
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

Instruction *init_instruction(eOPCode op_code, Arg *arg1, Arg *arg2)
{
    Instruction *instr = malloc(sizeof(Instruction));
    instr->op_code = op_code;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    return instr;
}

/* Returns instruction set to perform cast
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
*/
Instruction *gen_cast_instr(Arg *arg, eOPCode desired_type)
{
    //TODO: implement this ... at all
    return init_instruction(CAST, arg, NULL);
}

/* Returns instruction set to perform multiplication 
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
*/
Instruction *gen_additive_instr(Arg *arg1, Arg *arg2)
{
    //TODO: implement this more fully
    return init_instruction(ADD, arg1, arg2);
}

/* Returns instruction set to perform multiplication 
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
*/
Instruction *gen_multiplicative_instr(Arg *arg1, Arg *arg2)
{
    //TODO: implement this more fully 
    return init_instruction(MULT, arg1, arg2);
}

