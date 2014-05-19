#ifndef CONVERTER
#define CONVERTER

#include <glib.h>
#include <stdbool.h>
#include <assert.h>
#include "register.h"
#include "repr_utils.h"
#include "../InterCodeUtils/symbol_table.h"
#include "../InterCodeUtils/inter_code_gen.h"

// Define constants for register names
//static const int NUM_REGISTERS = 6;
#define NUM_REGISTERS 6
struct Register *EBP_REGISTER;
struct Register *ESP_REGISTER;
FILE *out_file;

// Sizes
static const int CHAR_SIZE = 1;
static const int INT_SIZE = 4;
static const int LONG_SIZE = 4;
static const int DOUBLE_SIZE = 8;
static const int BOOL_SIZE = 4;

// Options
static const char *DWORD_OPTION = "DWORD";

// Printing Utils
static const char *NUM_PRINT_FMT = "num_print db '%d',10,0";
static const char *NUM_PRINT = "num_print";

struct Register *REGISTERS[NUM_REGISTERS];
static int CUR_STACK_OFFSET = 0;
// EAX, EBX, ECX, EDX, ESI, EDI


void compile(GPtrArray *instr_list, GHashTable *symbol_table,
             int num_instrs, char *out_file);
int get_byte_size(eType type);
#endif
