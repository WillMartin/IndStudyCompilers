#include "symbol_table.h"

// Hold on to the table so we don't have to pass it everywhere.
GHashTable *_table;

GHashTable *gen_symbol_table()
{
    _table = g_hash_table_new_full(g_int_hash, 
                                   g_int_equal, 
                                   NULL, 
                                   g_free);
    return _table;
}

char *get_symbol(int key)
{
    return ((attr*) g_hash_table_lookup(_table, GINT_TO_POINTER(key)))->symbol;
}

void put_symbol(int key, char *symbol)
{
    g_hash_table_insert(_table, GINT_TO_POINTER(key), symbol);
}
