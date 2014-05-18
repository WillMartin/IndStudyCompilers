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
    BOOL,
} eType;

// Entry in symbol table has symbol lexeme, type info
typedef struct Identifier
{
    char *symbol;
    eType type;

    bool on_stack; // Whether the most current version is here
    // If a current version must be kept on stack and only grabbed from there
    // primarily for while-loops.
    bool force_on_stack;
    // This is "its" place on the stack. Should not change!
    int offset; 
    GList *address_descriptor; // Of type Register
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
