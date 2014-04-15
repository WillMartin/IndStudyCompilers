#include <glib.h>

typedef struct
{
    char *symbol;
} attr;

GHashTable *gen_symbol_table();
/* Keys will be ENUMs (aka ints) */
char *get_symbol(int key);
void put_symbol(int key, char *symbol);
