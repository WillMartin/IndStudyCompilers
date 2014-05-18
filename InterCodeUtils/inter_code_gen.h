/* Interface defining code to generate intermediate representation.
   The intermediate representation will be using the Indirect-Triplet approach */
#ifndef INTER_CODE_GEN
#define INTER_CODE_GEN

#include "symbol_table.h"
#include <assert.h>
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
    GOTO,   // unconditional goto
    NOP,    // No-op for ease
    // Relative operators (Cond. GOTO)
    EQ,
    NEQ,
    LT,
    GT,
    LEQ,
    GEQ,
} eOPCode;

// For debugging only
static const char* OP_CODE_REPRS[] = {"=", "+", "CAST", "/", "*", "-", "-(UNARY)",
                      "GOTO", "NOP", "==", "!=", "<", ">", "<=", ">=" };

typedef struct Constant
{
    eType type;
    union
    {
        int int_val;
        double float_val;
        char *str_val;
        long long_val;
        bool bool_val;
    };
} Constant;

typedef enum eArgType
{
    CONST, 
    INSTR, // A pointer to an Instruction struct 
    IDENT, // A pointer to an Ident struct (symbol_table.h)
    BOOLEAN_EXPR, // Just used for transporting true/false lists up
} eArgType;

// Args can either be constants (nums), statics (strings), symbols (ptr to symbol table), or other instruction (ptr to Instruction)
typedef struct Arg 
{
    eArgType type;     
    union 
    {
        Constant *const_val; // For now we'll just allow integer constants
        // Should only be used for jumps
        struct Instruction *instr_val; 
        Identifier *ident_val;
    };
    
    // For jumps/conditionals
    GList *true_list;
    GList *false_list;
} Arg;

typedef enum eActionType
{
    FORCE_ID_STACK,
    RELEASE_ID_STACK,
} eActionType;

typedef struct Action
{
    eActionType type;
    Identifier *id;
} Action;

// Represents an instruction as a generic Indirect-Triplet
typedef struct Instruction
{
    eOPCode op_code;
    Arg *arg1;
    Arg *arg2;
    // Where the result of the instruction should be stored
    Identifier *result;
    // For jumps
    struct Instruction *goto_addr;
    char *label;
    // Any other actions that should be done when assembling this instr. For
    // example locking or unlocking variables. Type: Action
    GList *actions; 
} Instruction;

bool is_relative_op(eOPCode op_code);

GPtrArray *init_instr_list();
void add_instr(GPtrArray *instr_list, int *num_instrs, Instruction* instr);
Instruction *get_instr(GPtrArray *instr_list, int num_instrs, int index);
void print_instr_list(GPtrArray *instr_list, int num_instrs);

Instruction *init_instruction(eOPCode op_code, Arg *arg1, Arg *arg2, Identifier *result);
Instruction *init_goto_instr(Instruction *goto_addr);
Instruction *init_cond_instr(eOPCode op_code, Arg *arg1, Arg *arg2, Instruction *goto_addr);
Instruction *init_assign_instr(Arg *arg1, Identifier *result);
Instruction *init_nop_instr();

GList *add_action_to_instr_range(GPtrArray *instr_list, int num_instrs, 
                                 int s_id, int e_id, eActionType type);

void *add_action_to_instr(Instruction *instr, Action *act);

Instruction *gen_additive_instr(GHashTable *symbol_table, Arg *arg1, Arg *arg);
Instruction *gen_subtractive_instr(GHashTable *sym_table, Arg *arg1, Arg *arg2);
Instruction *gen_multiplicative_instr(GHashTable *symbol_table, Arg *arg1, Arg *arg);

Arg *init_arg(eArgType type, void *val);

// Backtracking functions
void *print_list(GList *l);
GList *merge_lists(GList *l1, GList *l2);
GList *make_list(int instr_idx);
void back_patch(GPtrArray *instr_list, int num_instrs, GList *list, int instr_idx);

#endif // INTER_CODE_GEN
