#include <glib.h>
#include <stdbool.h>
#include <assert.h>
#include "../InterCodeUtils/symbol_table.h"
#include "../InterCodeUtils/inter_code_gen.h"

#define NUM_REGISTERS 6

typedef struct Register
{
    char *repr;
    GList *variables_held; // For identifiers held in registers
} Register;

typedef struct Address
{   
    eAddressType type; 
    union
    {
        Register *reg_addr_val;
        struct Identifier *ident_addr_val;
    };
} Address;

void compile(GPtrArray *instr_list, GHashTable *symbol_table,
             int num_instrs, char *out_file);
int get_byte_size(eType type);
