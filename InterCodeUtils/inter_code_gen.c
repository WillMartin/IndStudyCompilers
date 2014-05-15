#include "inter_code_gen.h"

static int NEXT_LABEL = 0;

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
                sprintf(repr, "%s", arg->instr_val->label);
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

bool is_relative_op(eOPCode op_code)
{
    if (op_code == EQ || op_code == NEQ ||
        op_code == GT || op_code == LT  ||
        op_code == GEQ || op_code == LEQ)
    {
        return true;
    }
    return false;
}


void print_instr(Instruction *instr)
{
    char *result_repr, *arg1_repr, *arg2_repr;
    arg1_repr = get_arg_repr(instr->arg1);
    arg2_repr = get_arg_repr(instr->arg2);
    if (is_relative_op(instr->op_code))
    {
        printf("IF %s %s %s-> GOTO %s\n", arg1_repr,
               OP_CODE_REPRS[instr->op_code], arg2_repr, instr->goto_addr->label);
    }
    else if (instr->op_code == GOTO) 
    { 
        char *label = instr->label != NULL ? instr->label : "";
        printf("%s GOTO %s\n", label, instr->goto_addr->label);
    }
    else if (instr->op_code == NOP)
    {
        char *label = instr->label != NULL ? instr->label : "";
        printf("%s NOP\n", label);
    }
    else if (instr->op_code == ASSIGN)
    {
        result_repr = instr->result->symbol; 
        char *label = instr->label != NULL ? instr->label : "";
        printf("%s %s = %s\n", label, result_repr, arg1_repr);
    }
    else 
    { 
        result_repr = instr->result->symbol; 
        char *label = instr->label != NULL ? instr->label : "";
        printf("%s %s=%s %s %s\n", label, result_repr, arg1_repr,
               OP_CODE_REPRS[instr->op_code], arg2_repr);
    }

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

Instruction *init_base_instr(eOPCode op_code, Arg *arg1, Arg *arg2,
                              Identifier *result, Instruction *goto_addr)
{
    Instruction *instr = malloc(sizeof(Instruction));
    instr->op_code = op_code;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    instr->result = result;
    instr->goto_addr = goto_addr;
    instr->label = NULL;
    return instr;
}

Instruction *init_instr(eOPCode op_code, Arg *arg1, Arg *arg2,
                              Identifier *result)
{
    return init_base_instr(op_code, arg1, arg2, result, NULL);
}

Instruction *init_goto_instr(Instruction *goto_addr)
{
    return init_base_instr(GOTO, NULL, NULL, NULL, goto_addr);
}

Instruction *init_cond_instr(eOPCode op_code, Arg *arg1, Arg *arg2, 
                             Instruction *goto_addr)
{
    return init_base_instr(op_code, arg1, arg2, NULL, goto_addr);
}

Instruction *init_assign_instr(Arg *arg1, Identifier *result)
{
    return init_base_instr(ASSIGN, arg1, NULL, result, NULL);
}


Instruction *init_nop_instr() 
{
    return init_base_instr(NOP, NULL, NULL, NULL, NULL);
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
    return init_instr(ADD, arg1, arg2, temp);
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
    return init_instr(SUB, arg1, arg2, temp);
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
    return init_instr(MULT, arg1, arg2, temp);
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

char *get_next_label()
{
    char *label = malloc(5);
    sprintf(label, ".l%d", NEXT_LABEL);
    NEXT_LABEL += 1;
    return label;

}

/* Loops through the <list> of ints and inserts <instr_idx>
    as the target jump for each instruction indexed by <list> */
void back_patch(GPtrArray *instr_list, int num_instrs, GList *list, int instr_idx)
{
    printf("BACK PATCHING %d into\n", instr_idx);
    print_list(list);
    Instruction *op_instr = get_instr(instr_list, num_instrs, instr_idx);
    assert(op_instr != NULL);

    // Make sure that the op_instr has a label, if it doesn't make one
    if (op_instr->label == NULL) { op_instr->label = get_next_label(); }

    for (; list!=NULL; list=list->next)
    {
        int cur_idx = GPOINTER_TO_INT(list->data);
        Instruction *goto_instr = get_instr(instr_list, num_instrs, cur_idx);

        assert(goto_instr != NULL);
        // Could be cond
        //assert(goto_instr->op_code == GOTO);
        //assert(goto_instr->arg1 == NULL);

        Arg *arg = init_arg(INSTR, op_instr);
        goto_instr->goto_addr = op_instr;
    }
}
