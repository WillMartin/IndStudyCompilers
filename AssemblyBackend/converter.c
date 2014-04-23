#include "converter.h"

// Define constants for register names
static const char *EBP_REGISTER = "ebp"; // Base (code) pointer reg
static const char *ESP_REGISTER = "esp"; // Stack pointer reg

// Instructions
static const char *MOVE_INSTR = "mov";
static const char *PUSH_INSTR = "push";
static const char *SUB_INSTR = "sub";
static const char *ADD_INSTR = "add";

// Sizes
static const int CHAR_SIZE = 1;
static const int INT_SIZE = 4;
static const int LONG_SIZE = 4;
static const int DOUBLE_SIZE = 8;

Register registers[NUM_REGISTERS];
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

char *get_assembly_type(eType type)
{
    char *repr = malloc(2 * sizeof(char));
    
    switch (type)
    {
        case INTEGER:
            repr = "DD";
            break;
        case DOUBLE:
            repr = "DD";
            break;
        /* TODO: Long
        case LONG:
            repr = "D";
            break;
        */
        default:
            break;
    }
    return repr;
}


void write_1instr(FILE *fp, const char *command, char *arg)
{
    fprintf(fp, "\t%s %s\n", command, arg); 
}

void write_2instr(FILE *fp, const char *command,
                  char *arg1, char *arg2)
{
    fprintf(fp, "\t%s %s, %s\n", command, arg1, arg2); 
}

void write_3instr(FILE *fp, const char *command, 
                  char *result, char *arg1, char *arg2)
{
    fprintf(fp, "\t%s %s, %s, %s\n", command, result, arg1, arg2); 
}

/**
 *  Give each value in the symbol table an address and record it.
 */
GHashTable *address_local_variables(FILE *fp, GHashTable *symbol_table)
{


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
void write_local_variables(FILE *fp, GHashTable *address_table)
{
    //TODO: change this to local identifiers eventually
    GList *local_ids = get_all_identifiers(address_table);

    int offset = 0;
    for (; local_ids != NULL; local_ids = local_ids->next)
    {
        int new_offset = ((Identifier *) local_ids->data)->offset;
        if (new_offset > offset)
        {
            offset = new_offset;
        }

        //write_2instr(fp, SUB_INSTR, ESP_REGISTER, char_offset);
    }

    if (offset > 0)
    {
        char *char_offset = char_int(offset);
        write_2instr(fp, SUB_INSTR, ESP_REGISTER, char_offset);
    }
}

// Push's down stack frame and adds memory for all local variables
void write_activation_record(FILE *fp, GHashTable *symbol_table)
{
}

// I don't think this is a great technique, but I want to get something working
void init_data_section(GHashTable *symbol_table, FILE *fp)
{
    GList *idents = get_all_identifiers(symbol_table);
    
    fprintf(fp, "SECTION .data\n");
    for (; idents!=NULL; idents = idents->next)
    {
        Identifier *id = (Identifier *) idents->data;
        char *repr = get_assembly_type(id->type);
        fprintf(fp, "\t%s\t%s\t0", id->symbol, repr);
    }
}


void init_registers()
{
    for (int i=0; i < NUM_REGISTERS; i++)
    {
        registers[i] = (Register) { .used=false, .memory_used=0 };
    }
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
    Arg *cur_arg = instr->arg1;

    char *arg_reprs[2];
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
                // Recurse!
                arg_reprs[i] = "poop";
                break;
            case IDENT:
                // TODO: make this good
                arg_reprs[i] = cur_arg->ident_val->symbol;
                break;
            default:
                printf("ERROR: Traverse Instructions, unknown type\n");
                break;
        }
        cur_arg = instr->arg2;
    }

    char *op_code = repr_op_code(instr->op_code);

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
    //init_data_section(symbol_table, file);
    write_local_variables(file, symbol_table);

    Instruction *cur_instr = get_instr(instr_list, num_instrs, 0);

    fclose(file);
}



