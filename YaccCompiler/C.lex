/* WDM Created 4/14/2014 */
 /* Manifest constants go here (Can be returned later) */
%{ 
#include <stdio.h>
#include "y.tab.h"
%}
 
 /* This tells flex to read only one input file */
%option noyywrap

 /* Regex definitions from grammar found in IOS/IEC C standard, Appen. A*/
delim       [ \t\n]
ws          {delim}+
digit       [0-9]
nondigit    [a-zA-Z_]
identifier  {identifier}-{nondigit}

integer     {digit}+

 /* Now match and do actions */
%%
{ws}        { /* Don't do anything on white space */ }
{digit}     { yylval = atoi(yytext); return DIGIT; }
{integer}   { yylval = atoi(yytext); return DIGIT; }
.           { /* For anything else, do nothing (TODO: Errors) */ }

%%

