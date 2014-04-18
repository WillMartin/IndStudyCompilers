#include "symbol_table.h"

GHashTable *init_symbol_table()
{
    return g_hash_table_new_full(g_str_hash, 
                                 g_str_equal, 
                                 NULL, 
                                 g_free);
}

Identifier *get_identifier(GHashTable *sym_table, char* sym)
{
    return (Identifier*) g_hash_table_lookup(sym_table, sym);
}

Identifier *put_symbol(GHashTable *sym_table, char *symbol, eType type)
{
    Identifier *id = malloc(sizeof(Identifier));
    id->symbol = symbol;
    id->type = type;
    g_hash_table_insert(sym_table, symbol, id);
    return id;
}
