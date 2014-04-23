#include <glib.h>
#include <stdbool.h>
#include <assert.h>
#include "../InterCodeUtils/symbol_table.h"
#include "../InterCodeUtils/inter_code_gen.h"

#define NUM_REGISTERS 6

typedef struct Register
{
    bool used;
    int memory_used;
} Register;

void compile(GPtrArray *instr_list, GHashTable *symbol_table,
             int num_instrs, char *out_file);
int get_byte_size(eType type);
