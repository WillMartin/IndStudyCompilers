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
extern char *strdup();
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
real_val        {integer_val}\.{integer_val}?
string_val      \"[^\"]*\"
long_val        {integer_val}l

identifier      {nondigit}+


 /* Now match and do actions */
%%
{ws}        { /* Don't do anything on white space */ }
{comment}   { /* Ignore comments too */ }

{integer_val}   { yylval.ival = atoi(yytext); return INT_LITERAL;    }
{real_val}      { yylval.dval = atof(yytext); return REAL_LITERAL;   }
{long_val}      { yylval.dval = atol(yytext); return LONG_LITERAL;   }
{string_val}    { yylval.cval = strdup(yytext); return CHAR_LITERAL; }

    /* Types - char, float, int, long */
double      { return DOUBLE_TYPE_TOKEN; }
int         { return INT_TYPE_TOKEN;    }
long        { return LONG_TYPE_TOKEN;   }
char        { return CHAR_TYPE_TOKEN;   }
bool        { return BOOL_TYPE_TOKEN;   }

if          { return IF_TOKEN;    }
else        { return ELSE_TOKEN;  }
while       { return WHILE_TOKEN; }
true        { return TRUE_TOKEN;  }
false       { return FALSE_TOKEN; }
print       { return PRINT_TOKEN; }

\|\|        { return OR_TOKEN;  }
&&          { return AND_TOKEN; }
==          { yylval.cval = strdup(yytext); return EQUALITY_TOKEN;    }
!=          { yylval.cval = strdup(yytext); return EQUALITY_TOKEN;    }
\<=         { yylval.cval = strdup(yytext); return RELATIONAL_TOKEN;  }
>=          { yylval.cval = strdup(yytext); return RELATIONAL_TOKEN;  }
\<          { yylval.cval = strdup(yytext); return RELATIONAL_TOKEN;  }
>           { yylval.cval = strdup(yytext); return RELATIONAL_TOKEN;  }

=           { return ASSIGN_OP; }

    /* Important this this goes below any reserved keys (matches all char strings) */
{identifier}    { yylval.cval = strdup(yytext); return IDENTIFIER; }

    /* For anything else, pass it to YACC to deal with */
.           { return yytext[0]; }

%%

