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


void write_1instr(FILE *fp, char *command, char *arg)
{
    fprintf(fp, "\t%s %s\n", command, arg); 
}

void write_2instr(FILE *fp, char *command,
                  char *arg1, char *arg2)
{
    fprintf(fp, "\t%s %s, %s\n", command, arg1, arg2); 
}

void write_3instr(FILE *fp, char *command, 
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

/**
 *  Write local variables to the stack. 
 */
void write_local_variables(FILE *fp, GHashTable *address_table)
{
    //TODO: change this to local identifiers eventually
    GList *local_ids = get_all_identifiers(address_table);

    for (; local_ids != NULL; local_ids = local_ids->next)
    {
        int size = get_byte_size(((Identifier *) local_ids->data)->type);
        
    }
}

// Push's down stack frame and adds memory for all local variables
void write_activation_record(FILE *fp, GHashTable *symbol_table)
{
}

// Write instructions
void write_code(FILE *fp, GPtrArray *instr_list)
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

// Recursively descend instruction list
void traverse_instructions(Instruction *instr, FILE fp)
{
         
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
    init_data_section(symbol_table, file);

    Instruction *cur_instr = get_instr(instr_list, num_instrs, 0);

    fclose(file);
}



