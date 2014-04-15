/* WDM Created 4/14/2014 */
%{
    /* Declarations: includes, globals */
    #include <stdio.h>
    #include "symbol_table.h"
%}

/* Define start symbol */
%start expr_list

/* Declare what types can be returned in yylval? */
%union 
{ 
  int ival; 
  double dval; 
  char *cval;        
}
/* Define a type for non-terms (?) */
%type <ival> expr_list

/* Define tokens */
%token <ival> INT_LITERAL
%token <dval> DOUBLE_LITERAL 
%token <cval> CHAR_LITERAL

%token DOUBLE_TYPE INT_TYPE OPEN_BRACK CLOSE_BRACK 

/* Define operators and their precedence */

%%
 /* Rules */
expr_list:                /* empty */
            | expr_list expr
            | expr
            ;
 
expr:       
            literal
            ;

literal:    number
            | string
            ;
                        
number: 
            INT_LITERAL
            {
                /* Semantic actions for seeing a digit */
                printf("<int:%d>", $1);
            }
            |
            DOUBLE_LITERAL
            {
                printf("<double:%f>", $1);
            }
            ;
                
string:     CHAR_LITERAL
            {
                printf("<str:%s>", $1);
            }
            ;
%%

/* Can run the file directly (without yacc) */
int yyerror(const char *str)
{
    fprintf(stderr,"error: %s\n",str);
}
 
int yywrap()
{
    return 1;
} 
   
main()
{
    yyparse();
    printf("\n\n");
} 
