#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static int temp_number = 0;

// Added the list for allowing for nested tables.
typedef struct SymbolTableList
{
    GHashTable *table;
    struct SymbolTableList *next;
} SymbolTableList;

// Allowable types
typedef enum eType
{
    INTEGER,
    DOUBLE,
    CHAR,
    LONG,
} eType;

typedef enum eAddressType
{
    REGISTER_TYPE,
    IDENTIFIER_TYPE,
} eAddressType;

// Entry in symbol table has symbol lexeme, type info
typedef struct Identifier
{
    char *symbol;
    eType type;

    int offset; // Offset from start of location
    GList *address_descriptor; // Of type Address
} Identifier; 

GHashTable *init_symbol_table();
Identifier *get_identifier(GHashTable *sym_table, char* key);
Identifier *put_symbol(GHashTable *sym_table, char *symbol, eType type);
Identifier *get_temp_symbol();
void put_identifier(GHashTable *sym_table, Identifier *id);

void print_symbol_table(GHashTable *sym_table);

// Hack together
GList *get_all_identifiers(GHashTable *sym_table);
//TODO
//GList *get_local_identifiers(SymbolTableList *sym_tab_list);

#endif // SYMBOL_TABLE
