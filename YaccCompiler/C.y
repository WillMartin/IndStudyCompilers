/* WDM Created 4/14/2014 */
%{
    /* Declarations: includes, globals */
#include <stdio.h>
#include "symbol_table.h"
%}

/* Define start symbol */
%start primary_expression

/* Declare what types can be returned in yylval? */
%union 
{ 
  int ival; 
  double dval; 
  char *cval;        
}
/* Define a type for non-terms (?) */
%type <cval> id

/* Define tokens */
%token <ival> INT_LITERAL
%token <dval> DOUBLE_LITERAL 
%token <cval> CHAR_LITERAL
%token <cval> IDENTIFIER
%token <cval> ASSIGN_OP

%token DOUBLE_TYPE INT_TYPE OPEN_BRACK CLOSE_BRACK OPEN_PAREN CLOSE_PAREN COMMA

/* Define operators and their precedence */

%%
 /* Rules */
primary_expression:
              id 
            | literal
            | OPEN_PAREN expression CLOSE_PAREN

expression:       
              assignment_expression
            | expression COMMA assignment_expression 
            ;

assignment_expression:
              id ASSIGN_OP assignment_expression
              {
                    printf("Assign:(%s->)", $1);
              }
            |
            ;

literal:      number
            | string
            ;
                        
number:     INT_LITERAL
            {
                /* Semantic actions for seeing a digit */
                printf("<int:%d>", $1);
            }
            | DOUBLE_LITERAL
            {
                printf("<double:%f>", $1);
            }
            ;
                
string:     CHAR_LITERAL
            {
                printf("<str:%s>", $1);
            }
            ;

id:         IDENTIFIER
            {
                printf("<id:%s>", $1);
            };
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
    gen_symbol_table();
    yyparse();
    printf("\n\n");
} 
