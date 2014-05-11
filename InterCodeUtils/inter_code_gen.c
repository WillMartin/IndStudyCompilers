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
            sprintf(repr, "%s", c->str_val);
            break; 
        case BOOL:
            sprintf(repr, "%i", c->bool_val);
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

void print_instr(Instruction *instr)
{
    char *result_repr, *arg1_repr, *arg2_repr;
    arg1_repr = get_arg_repr(instr->arg1);
    arg2_repr = get_arg_repr(instr->arg2);
    if (instr->op_code != GOTO) { result_repr = instr->result->symbol; }
    else { result_repr = "NO RESULT"; }
    printf("%s<-%s:%s:%s\n", result_repr, OP_CODE_REPRS[instr->op_code], 
            arg1_repr, arg2_repr);
    free(arg1_repr);
    free(arg2_repr);
    // Don't free result repr as it's using it's symbol
}

void print_instr_list(GPtrArray *instr_list, int num_instrs)
{
    for (int i=0; i < num_instrs; i++)
    {
        Instruction *instr = (Instruction*) g_ptr_array_index(instr_list, i);
        print_instr(instr);
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

Arg *init_arg(eArgType type, void *val)
{
    Arg *arg = malloc(sizeof(Arg));
    arg->true_list = NULL;
    arg->false_list = NULL;
    arg->type = type;
    switch (type)
    {
        case CONST:
            arg->const_val = (Constant*) val;
            break;
        case IDENT:
            arg->ident_val = (Identifier*) val;
            break;
        case INSTR:
            arg->instr_val = (Instruction*) val;
            break;
        default:
            break;
    }
    return arg;
}

void *print_list(GList *l)
{
    printf("<");
    for(; l!=NULL; l=l->next)
    {
        printf("%d, ", GPOINTER_TO_INT(l->data));
    }
    printf(">\n");
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
    Instruction *op_instr = get_instr(instr_list, num_instrs, instr_idx);
    assert(op_instr != NULL);
    printf("Starting Back Patch\n");
    printf("Op instr: %d, Total number: %d\n", instr_idx, num_instrs);
    printf("patching into list:");
    print_list(list);
    print_instr_list(instr_list, num_instrs);

    for (; list!=NULL; list=list->next)
    {
        int cur_idx = GPOINTER_TO_INT(list->data);
        Instruction *goto_instr = get_instr(instr_list, num_instrs, cur_idx);

        assert(goto_instr != NULL);
        assert(goto_instr->op_code == GOTO);
        assert(goto_instr->arg1 == NULL);

        Arg *arg = init_arg(INSTR, op_instr);
        goto_instr->arg1 = arg;
    }
}
