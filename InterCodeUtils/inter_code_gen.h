/* Interface defining code to generate intermediate representation.
   The intermediate representation will be using the Indirect-Triplet approach */
#ifndef INTER_CODE_GEN
#define INTER_CODE_GEN

#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

struct Arg;
struct Instruction;

// All defined op-codes to be defined here. 
typedef enum eOPCode 
{
    ASSIGN,
    ADD,
    CAST,
    DIV,
    MULT,
    SUB,
    UMINUS, // unary minus sign
} eOPCode;

typedef struct Constant
{
    eType type;
    union
    {
        int int_val;
        double float_val;
        char *str_val;
        long long_val;
    };
} Constant;

typedef enum eArgType
{
    CONST, 
    INSTR, // A pointer to an Instruction struct 
    IDENT, // A pointer to an Ident struct (symbol_table.h)
} eArgType;

// Args can either be constants (nums), statics (strings), symbols (ptr to symbol table), or other instruction (ptr to Instruction)
typedef struct Arg 
{
    eArgType type;     
    union 
    {
        Constant const_val; // For now we'll just allow integer constants
        struct Instruction *instr_val;
        Identifier *ident_val;
    };
} Arg;


// Represents an instruction as a generic Indirect-Triplet
typedef struct Instruction
{
    eOPCode op_code;
    Arg arg1;
    Arg arg2;
} Instruction;

GPtrArray *init_instr_list();
void add_instr(GPtrArray *instr_list, int *num_instrs, Instruction* instr);
Instruction *get_instr(GPtrArray *instr_list, int num_instrs, int index);
void print_instr_list(GPtrArray *instr_list, int num_instrs);

Instruction *init_instruction(eOPCode op_code, Arg arg1, Arg arg2);
Instruction *gen_additive_instr(Arg arg1, Arg arg);
Instruction *gen_multiplicative_instr(Arg arg1, Arg arg);

#endif // INTER_CODE_GEN
