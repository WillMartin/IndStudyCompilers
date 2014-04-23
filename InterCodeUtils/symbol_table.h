#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

// Where the data is allocated
typedef enum eAllocLocation
{
    STATIC,
    STACK,
} eAllocLocation;

// Entry in symbol table has symbol lexeme, type info
typedef struct Identifier
{
    char *symbol;
    eType type;

    eAllocLocation location; 
    int offset; // Offset from start of location
} Identifier;

GHashTable *init_symbol_table();
Identifier *get_identifier(GHashTable *sym_table, char* key);
Identifier *put_symbol(GHashTable *sym_table, char *symbol, eType type);
void put_identifier(GHashTable *sym_table, Identifier *id);

// Hack together
GList *get_all_identifiers(GHashTable *sym_table);
//TODO
//GList *get_local_identifiers(SymbolTableList *sym_tab_list);

#endif // SYMBOL_TABLE
