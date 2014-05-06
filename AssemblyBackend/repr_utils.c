#include "repr_utils.h"

/* Returns a newly malloc'd NASM style indirect addressing of <reg> */
char *repr_addr_ind(char *reg)
{
    char *repr = malloc((strlen(reg) + 3) * sizeof(char));
    sprintf(repr, "[%s]", reg);
    return repr;
}

/* Returns a newly malloc'd NASM style multiplication of <reg> by <fact> */
char *repr_addr_mult(char *reg, int fact)
{
    // Assuming fact are less than 1,000 in this case
    char *repr = malloc(sizeof(strlen(reg)) + 5);
    sprintf(repr,"%s*%d", reg, fact);
    return repr;
}

/* Returns a newly malloc'd NASM style addition to <reg> of <offset> */
char *repr_addr_add(char *reg, int offset)
{
    // Assuming offsets are less than 1,000 in this case
    char *repr = malloc(sizeof(strlen(reg)) + 5);
    if (offset < 0) { sprintf(repr,"%s-%d", reg, offset); }
    else if (offset > 0) { sprintf(repr,"%s+%d", reg, offset); }
    else { sprintf(repr, "%s", reg); }

    return repr;
}

/* Returns a newly malloc'd NASM int constant of <x> */
char *repr_int(int x) 
{
    char *repr = malloc(sizeof(char)*10);
    sprintf(repr, "%d", x);
    return repr;
}

/* Returns a newly malloc'd NASM real constant of <x> */
char *repr_real(double x) 
{
    char *repr = malloc(sizeof(char)*10);
    sprintf(repr, "%f", x);
    return repr;
}

/* Returns a newly malloc'd string for any Constant arg <c> */
char *repr_const(Constant *c)
{
    char *repr;

    switch (c->type)
    {
        case INTEGER:
            repr = repr_int(c->int_val);
            break;
        case DOUBLE:
            repr = repr_real(c->float_val);
            break;
        case CHAR:
            repr = c->str_val;
            break;
        case LONG:
            // TODO: give long's their own method
            repr = repr_int(c->int_val); 
            break;
        default:
            printf("CHAR_CONSTANT ERROR: unknown type");
            repr = "UH OH";
            break;
    }
    return repr;
}

/* Returns a string constant for <op_code>, no free necessary! */
const char *repr_op_code(eOPCode op_code)
{
    const char *const_repr;
    switch (op_code)
    {
        case ASSIGN:
            const_repr = MOVE_INSTR;
            break;
        case ADD:
            const_repr = ADD_INSTR;
            break;
        case SUB:
            const_repr = SUB_INSTR;
            break;
        case MULT:
            const_repr = MULT_INSTR;
            break;
        default:
            const_repr = "NOT IMPLEMENTED";
            break;
    }
    return const_repr;
}


// Just get it's address for now, if they want what's 
// in it the caller can indirect it
char *repr_ident(Identifier *ident)
{
    char *esp_repr = repr_reg(ESP_REGISTER);
    char *repr = repr_addr_add(esp_repr, ident->offset);
    free(esp_repr);
    return repr;
}


// Returns the character representation for the arg.
// If it's a constant it's just the constant
// If it's an identifier return a reg if possible, otherwise the indirect
// value of the stack location.
// Either way it needs to be freed.
char *repr_arg(Arg *arg)
{
    char *repr;
    // Constant is pretty simple
    if (arg->type == CONST)
    {
        repr = repr_const(arg->const_val);
    }
    else if (arg->type == IDENT)
    {
        Identifier *id = arg->ident_val;
        // Check to see if it's already in a register - more efficient!
        // Only one of these will be used.
        Register *reg_loc = NULL; 
        GList *arg_addrs = id->address_descriptor;
        for (; arg_addrs!=NULL; arg_addrs=arg_addrs->next)
        {
            reg_loc = arg_addrs->data;
            break;
        }

        // Grab it from the register
        if (reg_loc != NULL)
        {
            repr = repr_reg(reg_loc);
        }
        // If the current value held in the stack is current then we're fine
        else if (arg->ident_val->on_stack)
        {
            char *stack_base = repr_reg(ESP_REGISTER);
            char *with_offset = repr_addr_add(stack_base, id->offset);
            repr = repr_addr_ind(with_offset);
            free(stack_base);
            free(with_offset);
        }
        // Indicates the variable is neither on the stack nor
        // in a register -> has no representation
        else { assert(false); }
    }
    else { assert(false); }
    return repr;
}


