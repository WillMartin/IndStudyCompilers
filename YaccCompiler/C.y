/* WDM Created 4/14/2014 */
%{
    /* Declarations: includes, globals */
#include <stdio.h>
#include "symbol_table.h"
#include "inter_code_gen.h"
    
    // Declare some useful globals when parsing
GPtrArray *instr_list;
int num_instrs;
GHashTable *symbol_table;
%}

/* Define start symbol */
%start primary_expression

/* Declare what types can be returned in yylval? */
%union 
{ 
  int ival; 
  double dval; 
  char *cval;        
  Identifier *idval;
}
/* Define a type for non-terms (?) */
%type <idval> id

/* Define tokens */
%token <ival> INT_LITERAL
%token <dval> DOUBLE_LITERAL 
%token <cval> CHAR_LITERAL
%token <cval> IDENTIFIER
%token <cval> ASSIGN_OP

%token DOUBLE_TYPE INT_TYPE 

/* Define operators and their precedence */

%%
 /* Rules */
primary_expression:
              id 
            | literal
            | '(' expression ')'

expression:       
              assignment_expression
            | expression ',' assignment_expression 
            ;

assignment_expression:
              id ASSIGN_OP assignment_expression
              {
                    //printf("Assign:(%s->)", $1);
                    printf("Assign:(%s, %p)\n", $1->symbol, $1);
                    Instruction *instr = malloc(sizeof(Instruction));
                    instr->op_code = ASSIGN;

                    Arg arg1;
                    arg1.type = IDENT;
                    arg1.ident_val = $1;

                    Arg arg2;
                    arg2.type = CONST; //???
                    arg2.const_val = 8;

                    instr->arg1 = arg1;
                    instr->arg2 = arg2;
                    add_instr(instr_list, &num_instrs, instr);
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
                Identifier *sym_id;
                sym_id = get_identifier(symbol_table, $1);

                if (sym_id == NULL)
                {
                    sym_id = put_symbol(symbol_table, $1, INTEGER);
                }

                printf("<id:%s>\n", $1);
                $$ = sym_id;
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
  

int main()
{
    // Init globals
    symbol_table = init_symbol_table();
    instr_list = init_instr_list();
    num_instrs = 0;

    yyparse();
 
    print_instr_list(instr_list, num_instrs);
} 
