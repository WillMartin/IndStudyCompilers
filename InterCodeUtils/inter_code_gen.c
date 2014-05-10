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

// Return value needs to be freed by caller 
// (all will be malloc'd for this reason)
char *get_arg_repr(Arg *arg)
{
    char *repr;
    if (arg == NULL)
    {
        repr = malloc(9 * sizeof(char));
        sprintf(repr, "NULL_ARG");
    }
    else
    {
        switch (arg->type)
        {
            case CONST:
                repr = get_constant_repr(arg->const_val);
                break;
            case INSTR:
                repr = malloc(6 * sizeof(char));
                sprintf(repr, "INSTR");
                break;
            case IDENT:;
                char *sym = arg->ident_val->symbol;
                repr = malloc((strlen(sym) + 1) * sizeof(char));
                sprintf(repr, "%s", sym);
                break;
            default:
                repr = malloc(15 * sizeof(char));
                sprintf(repr, "UNKNOWN: %d", arg->type);
                break;
        }
    }
    return repr;
}

void print_instr_list(GPtrArray *instr_list, int num_instrs)
{
    Instruction *instr;
    char *arg1_repr,*arg2_repr, *result_repr;
    for (int i=0; i < num_instrs; i++)
    {
        instr = (Instruction*) g_ptr_array_index(instr_list, i);
        arg1_repr = get_arg_repr(instr->arg1);
        arg2_repr = get_arg_repr(instr->arg2);
        result_repr = instr->result->symbol;
        printf("%s<-%s:%s:%s\n", result_repr, OP_CODE_REPRS[instr->op_code], 
                arg1_repr, arg2_repr);
        free(arg1_repr);
        free(arg2_repr);
        // Don't free result repr as it's using it's symbol
    }
}

Instruction *init_instruction(eOPCode op_code, Arg *arg1, Arg *arg2,
                              Identifier *result)
{
    Instruction *instr = malloc(sizeof(Instruction));
    instr->op_code = op_code;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    instr->result = result;
    return instr;
}

/* Returns instruction set to perform cast
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
Instruction *gen_cast_instr(Arg *arg, eOPCode desired_type)
{
    //TODO: implement this ... at all
    return init_instruction(CAST, arg, NULL);
}
*/

/* Returns instruction set to perform addition
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
*/
Instruction *gen_additive_instr(GHashTable *sym_table, Arg *arg1, Arg *arg2)
{
    //TODO: implement this more fully
    Identifier *temp = get_temp_symbol();
    // For now we're just saying everything can be an int
    temp->type = INTEGER;

    put_identifier(sym_table, temp);
    return init_instruction(ADD, arg1, arg2, temp);
}

/* Returns instruction set to perform addition
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
*/
Instruction *gen_subtractive_instr(GHashTable *sym_table, Arg *arg1, Arg *arg2)
{
    //TODO: implement this more fully
    Identifier *temp = get_temp_symbol();
    // For now we're just saying everything can be an int
    temp->type = INTEGER;

    put_identifier(sym_table, temp);
    return init_instruction(SUB, arg1, arg2, temp);
}

/* Returns instruction set to perform multiplication 
   Returns NULL if the operation is unsuccessful 
        (e.g. wrong arg types)
*/
Instruction *gen_multiplicative_instr(GHashTable *sym_table, Arg *arg1, Arg *arg2)
{
    //TODO: implement this more fully
    Identifier *temp = get_temp_symbol();
    // For now we're just saying everything can be an int
    temp->type = INTEGER;

    put_identifier(sym_table, temp);
    //TODO: implement this more fully 
    return init_instruction(MULT, arg1, arg2, temp);
}

/* Concatatenates <l1>, <l2> and returns the result */
GList *merge_lists(GList *l1, GList *l2)
{
    return g_list_concat(l1, l2);
}

/* Create a new GList containing only <instr_idx> */
GList *make_list(int instr_idx)
{
    return g_list_prepend(NULL, GINT_TO_POINTER(instr_idx));
}

/* Loops through the <list> of ints and inserts <instr_idx>
    as the target jump for each instruction indexed by <list> */
void back_patch(GPtrArray *instr_list, int num_instrs, GList *list, int instr_idx)
{
    Instruction *goto_instr = get_instr(instr_list, num_instrs, instr_idx);
    printf("Num instrs: %d, inst_idx: %d\n", num_instrs, instr_idx);
    assert(goto_instr != NULL);
    for (; list!=NULL; list=list->next)
    {
        int cur_idx = GPOINTER_TO_INT(list->data);
        Instruction *cur_instr = get_instr(instr_list, num_instrs, instr_idx);
        assert(cur_instr != NULL);
        assert(cur_instr->op_code == GOTO);
        assert(cur_instr->arg1 != NULL);

        Arg *arg = malloc(sizeof(Arg));
        arg->type = INSTR;
        arg->instr_val = goto_instr;

        cur_instr->arg1 = arg;
    }
}
