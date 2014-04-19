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
%start statement

/* Declare what types can be returned in yylval? */
%union 
{ 
  int ival; 
  double dval; 
  char *cval;        
  long lval;

  Arg argval;
  Identifier *idval;
  Instruction *insval;
  eType tval;
}
/* Define a type for non-terms (?) */
%type <idval> id
%type <argval> statement expression compound_statement assignment_expression block_item block_item_list declaration literal initializer
%type <tval> type_specifier

/* Define tokens */
%token <ival> INT_LITERAL
%token <dval> DOUBLE_LITERAL 
%token <cval> CHAR_LITERAL
%token <lval> LONG_LITERAL
%token <cval> IDENTIFIER
%token <cval> ASSIGN_OP

%token DOUBLE_TYPE INT_TYPE LONG_TYPE

/* Define operators and their precedence */

%%
 /* Rules */
 /* Hardcoded to get something work */
statement:
              compound_statement
            | expression ';'

compound_statement:
              '{' block_item_list '}'
            ;
block_item_list:
              block_item
            | block_item_list block_item
            ;

block_item:
              declaration
            | statement
            ;

primary_expression:
              id 
            | literal
            | '(' expression ')'

expression:       
              assignment_expression
            | primary_expression
            ;

assignment_expression:
              /* TODO: id should be unary-expression */
              id ASSIGN_OP expression 
              {
                  if ($1->type == UNINITIALIZED) { printf("ERROR: use of undeclared id: %s\n", $1->symbol); }

                  printf("Assign:(%s, %p)\n", $1->symbol, $1);
                  Instruction *instr = malloc(sizeof(Instruction));
                  instr->op_code = ASSIGN;

                  Arg arg1;
                  arg1.type = IDENT;
                  arg1.ident_val = $1;

                  instr->arg1 = arg1;
                  // assignment_expresssion will return an argument
                  instr->arg2 = $3;
                  add_instr(instr_list, &num_instrs, instr);

                  $$ = (Arg) { .type=INSTR, .instr_val=instr };
              }
            ;

declaration:
                /* Clearly a subset as there are many more possible declerations */
              type_specifier id ASSIGN_OP initializer ';'
              {
                  Identifier *id = $2;
                  //TODO: If ID exists, throw an error
                  if (id->type != UNINITIALIZED) { printf("ERROR: already declared id! %s", id->symbol); }

                  id->type = $1;
                  put_identifier(symbol_table, id);
                  Arg id_arg = { .type=IDENT, .ident_val=id };

                  Instruction *instr = malloc(sizeof(Instruction));
                  instr->op_code = ASSIGN;
                  instr->arg1 = id_arg;
                  instr->arg2 = $4;
                  add_instr(instr_list, &num_instrs, instr);

                  $$ = (Arg) { .type=INSTR, .instr_val=instr };
              }
              ;

 /* Declarator for now can just be ID, should be more 
    Moved to declaration for simplicitry. Can only initialize as int x = 5;
init_declaration_list:
              id
              {

              }
            |
              id ASSIGN_OP initializer
              {

              }
            ;
            */

/* For now, just allow initialization to a primary expression */
initializer:
            primary_expression                
            ;

type_specifier: 
              INT_TYPE      { $$ = INTEGER; }
            | DOUBLE_TYPE   { $$ = DOUBLE;  }
            | LONG_TYPE     { $$ = LONG;    }
            ;

literal:      INT_LITERAL
              {
                  Constant cons = { .type=INTEGER, .int_val=$1 };
                  $$ = (Arg) { .type=CONST, .const_val=cons };

              }
            | DOUBLE_LITERAL
              {
                  Constant cons = { .type=DOUBLE, .float_val=$1 };
                  $$ = (Arg) { .type=CONST, .const_val=cons };

              }
            | LONG_LITERAL
              {
                  Constant cons = { .type=LONG, .long_val=$1 };
                  $$ = (Arg) { .type=CONST, .const_val=cons };
              }
            | CHAR_LITERAL
              {
                  Constant cons = { .type=STRING, .str_val=$1 };
                  $$ = (Arg) { .type=CONST, .const_val=cons };
              }
            ;

id:         IDENTIFIER
            {
                Identifier *sym_id;
                sym_id = get_identifier(symbol_table, $1);

                if (sym_id == NULL)
                {
                    //sym_id = put_symbol(symbol_table, $1, INTEGER);
                    sym_id = malloc(sizeof(Identifier));
                    sym_id->symbol = $1;
                    // Set to NULL representing uninitialized type
                    sym_id->type = UNINITIALIZED; 
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
