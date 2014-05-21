#ifndef REPR_UTILS
#define REPR_UTILS

#include "../InterCodeUtils/symbol_table.h"
#include "../InterCodeUtils/inter_code_gen.h"
#include "converter.h"
#include "register.h"

// Instructions
static const char *MOVE_INSTR = "mov";
static const char *PUSH_INSTR = "push";
static const char *SUB_INSTR = "sub";
static const char *ADD_INSTR = "add";
static const char *MULT_INSTR = "imul";

static const char *REL_INSTR = "cmp";
static const char *JMP_INSTR = "jmp";
static const char *EQ_INSTR = "je";
static const char *NEQ_INSTR = "jne";
static const char *LT_INSTR = "jl";
static const char *GT_INSTR = "jg";
static const char *LEQ_INSTR = "jle";
static const char *GEQ_INSTR = "jge";

static const char  *NOP_INSTR = "NOP";

char *repr_addr_ind(char *reg);
char *repr_addr_mult(char *reg, int fact);
char *repr_addr_add(char *reg, int offset);
char *repr_int(int x);
char *repr_real(double x);
char *repr_const(Constant *c);
const char *repr_op_code(eOPCode op_code);
char *repr_ident(Identifier *ident);
char *repr_stack_from_offset(int offset);
char *repr_arg(Arg *arg);
#endif
