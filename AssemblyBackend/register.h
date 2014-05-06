#ifndef REGISTER
#define REGISTER

#include <glib.h>
#include "converter.h"
#include "../InterCodeUtils/symbol_table.h"
#include "../InterCodeUtils/inter_code_gen.h"

/* Basic code to hold register information, methods to manipulate
   them, print them, etc. */
typedef struct Register
{
    const char *repr;
    GList *variables_held; // For identifiers held in registers
} Register;

char *repr_reg(Register *reg);
GList *remove_reg_from_addrs(GList *addrs, Register *reg);
void print_registers(Register **regs, int num_regs);
void print_register_state(Register *reg);
#endif
