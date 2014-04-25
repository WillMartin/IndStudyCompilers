#include "converter.h"

// Define constants for register names
static const char *EBP_REGISTER = "ebp"; // Base (code) pointer reg
static const char *ESP_REGISTER = "esp"; // Stack pointer reg

//static const char *REGISTERS[] = { "eax", "ebx", "ecx", "edx", "esi", "edi" };

// Instructions
static const char *MOVE_INSTR = "mov";
static const char *PUSH_INSTR = "push";
static const char *SUB_INSTR = "sub";
static const char *ADD_INSTR = "add";
static const char *MULT_INSTR = "imul";

// Sizes
static const int CHAR_SIZE = 1;
static const int INT_SIZE = 4;
static const int LONG_SIZE = 4;
static const int DOUBLE_SIZE = 8;

// Options
static const char *DWORD_OPTION = "DWORD";

Register *registers[NUM_REGISTERS];
// EAX, EBX, ECX, EDX, ESI, EDI

// Init to start addresses
int STACK_ADDR;
int CODE_ADDR;


int get_byte_size(eType type)
{
    int size;
    switch (type)
    {
        case INTEGER:
            size = INT_SIZE;
            break;
        case CHAR:
            size = CHAR_SIZE;
            break;
        case DOUBLE:
            size = DOUBLE_SIZE;
            break;
        case LONG:
            size = LONG_SIZE;
            break;
        default:
            printf("COMPILE ERROR: Unknown Type");
            size = -1;
            break;
    }
    return size;
}

void write_1instr(FILE *fp, const char *command, char *arg)
{
    fprintf(fp, "\t%s %s\n", command, arg); 
}

void write_2instr_with_option(FILE *fp, const char *command, char*option,
                            char *arg1, char *arg2)
{


    fprintf(fp, "\t%s %s %s, %s\n", command, option, arg1, arg2); 
}

void write_2instr(FILE *fp, const char *command,
                  char *arg1, char *arg2)
{
    write_2instr_with_option(fp, command, "", arg1, arg2);
}

char *addr_ind(const char *reg)
{
    char *repr = malloc(sizeof(strlen(reg) + 3));
    sprintf(repr, "[%s]", reg);
    return repr;
}

char *addr_mult(const char *reg, int fact)
{
    // Assuming fact are less than 1,000 in this case
    char *repr = malloc(sizeof(strlen(reg)) + 5);
    sprintf(repr,"%s*%d", reg, fact);
    return repr;
}

char *addr_add(const char *reg, int offset)
{
    // Assuming offsets are less than 1,000 in this case
    char *repr = malloc(sizeof(strlen(reg)) + 5);
    if (offset < 0) { sprintf(repr,"%s-%d", reg, offset); }
    else if (offset > 0) { sprintf(repr,"%s+%d", reg, offset); }
    else { sprintf(repr, "%s", reg); }

    return repr;
}



char *char_int(int x) 
{
    char *repr = malloc(sizeof(char)*10);
    sprintf(repr, "%d", x);
    return repr;
}

char *char_double(double x) 
{
    char *repr = malloc(sizeof(char)*10);
    sprintf(repr, "%f", x);
    return repr;
}

char *char_const(Constant *c)
{
    char *repr;

    switch (c->type)
    {
        case INTEGER:
            repr = char_int(c->int_val);
            break;
        case DOUBLE:
            repr = char_double(c->float_val);
            break;
        case CHAR:
            repr = c->str_val;
            break;
        case LONG:
            // TODO: give long's their own method
            repr = char_int(c->int_val); 
            break;
        default:
            printf("CHAR_CONSTANT ERROR: unknown type");
            repr = "UH OH";
            break;
    }
    return repr;
}


/**
 *  Make room for local variables
 */
void init_stack_variables(FILE *fp, GHashTable *address_table)
{
    //TODO: change this to local identifiers eventually
    GList *local_ids = get_all_identifiers(address_table);

    // Just push uninitialized space onto the stack
    int offset = 0;
    int data_size = 0;
    Identifier *cur_id;
    for (; local_ids != NULL; local_ids = local_ids->next)
    {
        cur_id = (Identifier *) local_ids->data;
        data_size = get_byte_size(cur_id->type);

        // Doesn't matter what we push on.
        write_1instr(fp, PUSH_INSTR, registers[0]->repr);
        offset += data_size;
        cur_id->offset = offset;
    }

    /* PUSH adjusts the stack pointer correctly
    if (offset > 0)
    {
        char *char_offset = char_int(offset);
        write_2instr(fp, SUB_INSTR, ESP_REGISTER, char_offset);
    }
    */
}

// TODO: Better place to put these?
void init_registers()
{
    for (int i=0; i < NUM_REGISTERS; i++)
    {
        registers[i] = malloc(sizeof(Register));
        // As specified in Docs, an empty Glist* is NULL
        registers[i]->variables_held = NULL;
    }

    // Manually give them reprs
    registers[0]->repr = "eax";
    registers[1]->repr = "ebx";
    registers[2]->repr = "ecx";
    registers[3]->repr = "edx";
    registers[4]->repr = "esi";
    registers[5]->repr = "edi";
}

char *repr_op_code(eOPCode op_code)
{
    char *repr;
    switch (op_code)
    {
        case ASSIGN:
            repr = MOVE_INSTR;
            break;
        case ADD:
            repr = ADD_INSTR;
            break;
        case SUB:
            repr = SUB_INSTR;
            break;
        case MULT:
            repr = MULT_INSTR;
            break;
        default:
            repr = "NOT IMPLEMENTED";
            break;
    }
    return repr;
}

// Recursively descend instruction list
// Set pointers to NULL once they've been written
// Do the postorder traversal
void traverse_instructions(FILE *fp, Instruction *instr)
{
    bool first_arg_is_reg = false;
    char *arg_reprs[2];
    Arg *cur_arg = instr->arg1;
    for (int i=0; i < 2; i++)
    {
        if (cur_arg == NULL)
        {
            printf("WARNING: Argument empty\n");
            arg_reprs[i] = NULL;
            continue;
        }

        switch (cur_arg->type)
        {
            case CONST:
                arg_reprs[i] = char_const(cur_arg->const_val);
                break;
            case INSTR:            
                // If this is the first argument and it's an instruction then we 
                // know that it will result in an answer in the first register.
                if (i == 0)
                {
                    first_arg_is_reg = true;
                }

                // Recurse! For now assume arg_reprs[0] result is in reg1 and
                // arg_reprs[1] is in reg2
                arg_reprs[i] = registers[i]->repr;
                
                // Now do the work to store the result in the correct register
                traverse_instructions(fp, cur_arg->instr_val);
                break;
            case IDENT:
                // Follow the pointer to get it from the stack
                arg_reprs[i] = addr_ind(addr_add(ESP_REGISTER, cur_arg->ident_val->offset));
                break;
            default:
                // Should never happen
                assert(false);
                break;
        }
        cur_arg = instr->arg2;
    }

    char *op_code = repr_op_code(instr->op_code);
    // If the op_code is an assignment then we want to place the second arg
    // into the first arg (Needs to be an ident.)
    if (instr->op_code == ASSIGN)
    {
        // The issue is that normally things to evaluate into the second argument
        // whereas here we know it will be in the first 
        // The issue is that the second argument will evaluate into the second
        // register. So let it know that !
    }
    // If not an assignment then we need the first arg to be a register.
    // If this is not the case (either it's a constant or an ident) then we need
    // to manually move it to the register. We've also grabbed any info from
    // the registers we'll shift into so we can overwite them.
    else if (!first_arg_is_reg)
    {
        write_2instr(fp, repr_op_code(ASSIGN), registers[0]->repr, arg_reprs[0]);
        arg_reprs[0] = registers[0]->repr;
    }


    // First arg should NEVER be null. Second arg could be (inefficient assign)
    assert(arg_reprs[0] != NULL);

    if (arg_reprs[1] == NULL)
    {
        write_1instr(fp, op_code, arg_reprs[0]);
    }
    else
    {
        write_2instr(fp, op_code, arg_reprs[0], arg_reprs[1]);
    }
}

char *get_assem_arg_repr(Arg *arg)
{
    assert(arg != NULL); 

    char *repr;
    switch (arg->type)
    {
        case CONST:
            repr = char_const(arg->const_val);
            break;
        case IDENT:
            // Just get it's address for now, if they want what's 
            // in it the caller can indirect it
            repr = addr_add(ESP_REGISTER, arg->ident_val->offset);
            break;
        default:
            // Should never happen
            assert(false);
            break;
    }
    return repr;
}

/* Naive way of compiling. Everything is loaded, operated on, then retured to 
    its proper stack location */
void stack_compile(GPtrArray *instr_list, GHashTable* symbol_table,
                   int num_instrs, FILE *fp)
{
    for (int i=0; i<num_instrs; i++)
    {
        Instruction *instr = get_instr(instr_list, num_instrs, i);
        if (instr->op_code == ASSIGN)
        {
            // Simple, just dump the first argument into the stack location 
            // of the result
            char *result_loc = addr_ind(addr_add(ESP_REGISTER, instr->result->offset)); 
            char *arg_repr;
            bool requires_option = false;
            // In an assignment only the first argument is populated
            switch(instr->arg1->type)
            {
                case CONST:
                    // Nice and easy, just get its string repr
                    arg_repr = char_const(instr->arg1->const_val);
                    requires_option = true;
                    break;
                case IDENT:
                    // Note: Move operations cannot be mem->mem
                    // Get addr of its ptr.
                    arg_repr = addr_ind(addr_add(ESP_REGISTER, instr->arg1->ident_val->offset)); 
                    // Then load it
                    write_2instr(fp, MOVE_INSTR, registers[0]->repr, arg_repr);
                    // Now pt the repr to the newly loaded instr
                    arg_repr = registers[0]->repr;
                    break;
                // I think this should never happen (includes INSTR)
                default:
                    assert(false);
            }
            // If the second argument is a constant then we need an option
            if (requires_option)
            {
                write_2instr_with_option(fp, MOVE_INSTR, DWORD_OPTION, result_loc, arg_repr);
            }
            else
            {
                write_2instr(fp, MOVE_INSTR, result_loc, arg_repr);
            }
        } 
        // Non-assign instructions require more work.
        else
        {
            // Get the first instr and load it into register 0
            char *arg_repr = get_assem_arg_repr(instr->arg1);
            // Because we're doing a move we want whatever is in the memory slot (not the address itself)
            if (instr->arg1->type == IDENT) { arg_repr = addr_ind(arg_repr); }
            write_2instr(fp, MOVE_INSTR, registers[0]->repr, arg_repr);

            // Now get the second arguments repr and do the operation
            arg_repr = get_assem_arg_repr(instr->arg2);
            if (instr->arg2->type == IDENT) { arg_repr = addr_ind(arg_repr); }
            write_2instr(fp, repr_op_code(instr->op_code), registers[0]->repr, arg_repr);
 
            // The final step is calculating where to store the result
            char *result_loc = addr_ind(addr_add(ESP_REGISTER, instr->result->offset)); 
            // Again just using register 0 for now
            write_2instr(fp, MOVE_INSTR, result_loc, registers[0]->repr);
        }
    }
}

void write_header(FILE *fp)
{
    fprintf(fp, "SECTION .text\n");
    fprintf(fp, "\tglobal _start\n");
    fprintf(fp, "_start:\n");
}

void write_exit(FILE *fp)
{
    // Set exit code 0 - OK
    fprintf(fp, "\tmov ebx, 0\n");
    // Send the exit command to kernel
    fprintf(fp, "\tmov eax, 1\n");
    // Use the 0x80 interrrupt to call kernel
    fprintf(fp, "\tint 0x80\n");

}

void compile(GPtrArray *instr_list, GHashTable* symbol_table,
             int num_instrs, char *out_file)
{
    if (num_instrs <= 0)
    {
        printf("Empty Instruction Set: No output created\n");
        return;
    }

    FILE *file = fopen(out_file, "w+");
    init_registers();
    write_header(file);
    init_stack_variables(file, symbol_table);
    stack_compile(instr_list, symbol_table, num_instrs, file);
    write_exit(file);
    fclose(file);
}



