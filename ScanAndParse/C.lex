/* WDM Created 4/14/2014 */
 /* Manifest constants go here (Can be returned later) */
%{ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Important that it goes in this order (otherwise structrs from symbol_table are declared undefined)
#include "InterCodeUtils/symbol_table.h"
#include "InterCodeUtils/inter_code_gen.h"
#include "y.tab.h"

extern int yylex();
%}
 
 /* This tells flex to read only one input file 
 %option noyywrap */

 /* Regex definitions from grammar found in IOS/IEC C standard, Appen. A*/
delim           [ \t\n]
ws              {delim}+
line_comment    \/\/[^\n]*\n
long_comment    \/\*.*\*\/
comment         {line_comment}|{long_comment}

nondigit        [a-zA-Z_]
digit           [0-9]

integer_val     {digit}+
double_val      {integer_val}\.{integer_val}?
string_val      \"[^\"]*\"
long_val        {integer_val}l

identifier      {nondigit}+


 /* Now match and do actions */
%%
{ws}        { /* Don't do anything on white space */ }
{comment}   { /* Ignore comments too */ }

{integer_val}   { yylval.ival = atoi(yytext); return INT_LITERAL; }
{double_val}    { yylval.dval = atof(yytext); return DOUBLE_LITERAL; }
{long_val}      { yylval.dval = atol(yytext); return LONG_LITERAL; }
{string_val}    { yylval.cval = strdup(yytext); return CHAR_LITERAL; }

    /* Types - TODO: char, float, int, long */
double      { return DOUBLE_TYPE; }
int         { return INT_TYPE;    }
long        { return LONG_TYPE;   }
char        { return CHAR_TYPE;   }

    /* TODO: Add a bunch of other assign operators */
=           { return ASSIGN_OP; }

    /* Important this this goes below any reserved keys (matches all char strings) */
{identifier}    { yylval.cval = strdup(yytext); return IDENTIFIER; }

    /* For anything else, pass it to YACC to deal with */
.           { return yytext[0]; }

%%

