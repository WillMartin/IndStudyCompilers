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

void put_identifier(GHashTable *sym_table, Identifier *id)
{
    g_hash_table_insert(sym_table, id->symbol, id);
}

Identifier *put_symbol(GHashTable *sym_table, char *symbol, eType type)
{
    Identifier *id = malloc(sizeof(Identifier));
    id->symbol = symbol;
    id->type = type;
    g_hash_table_insert(sym_table, symbol, id);
    return id;
}

Identifier *get_temp_symbol()
{
    Identifier *id = malloc(sizeof(Identifier));
    char *sym = malloc(4 * sizeof(char));
    sprintf(sym, "t%d", temp_number);
    id->symbol = sym;

    temp_number += 1;
    return id;
}

GList *get_all_identifiers(GHashTable *sym_table)
{
    return g_hash_table_get_values(sym_table);
}

void print_symbol_table(GHashTable *sym_table)
{
    GList *ids = get_all_identifiers(sym_table);

    for (; ids!=NULL; ids=ids->next)
    {
        Identifier *id = ids->data;
        printf("<sym:%s>\n", id->symbol);
    }
}
