#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Allowable types
// TODO: Add more than just int
typedef enum eType
{
    INTEGER,
    DOUBLE,
    CHAR,
    LONG,
    UNINITIALIZED,
} eType;

// Entry in symbol table has symbol lexeme, type info
typedef struct Identifier
{
    char *symbol;
    eType type;
} Identifier;

GHashTable *init_symbol_table();
/* Keys will be ENUMs (aka ints) */
Identifier *get_identifier(GHashTable *sym_table, char* key);
Identifier *put_symbol(GHashTable *sym_table, char *symbol, eType type);
void put_identifier(GHashTable *sym_table, Identifier *id);

#endif // SYMBOL_TABLE
