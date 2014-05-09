/* WDM Created 4/14/2014 */
%{
    /* Declarations: includes, globals */
#include <stdio.h>
#include "AssemblyBackend/converter.h"
#include "InterCodeUtils/symbol_table.h"
#include "InterCodeUtils/inter_code_gen.h"
    
    // Declare some useful globals when parsing
GPtrArray *instr_list;
int num_instrs;
// Should be reset at every new stack frame
// Assign then increment
int stack_offset;
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

  Arg *argval;
  Identifier *idval;
  Instruction *insval;
  eType tval;
}
/* Define a type for non-terms (?) */
%type <idval> id
%type <argval> statement expression compound_statement assignment_expression block_item block_item_list declaration literal initializer additive_expression multiplicative_expression primary_expression logical_or_expression logical_and_expression equality_expression relational_expression selection_statement iteration_statement
 
%type <tval> type_specifier

/* Define tokens */
%token <ival> INT_LITERAL
%token <dval> REAL_LITERAL 
%token <cval> CHAR_LITERAL
%token <lval> LONG_LITERAL
%token <cval> IDENTIFIER
%token <cval> ASSIGN_OP
%token <cval> EQUALITY_TOKEN
%token <cval> RELATIONAL_TOKEN

%token DOUBLE_TYPE_TOKEN INT_TYPE_TOKEN LONG_TYPE_TOKEN CHAR_TYPE_TOKEN IF_TOKEN ELSE_TOKEN WHILE_TOKEN OR_TOKEN AND_TOKEN

/* Define operators and their precedence */

%%
 /* Rules */
 /* Hardcoded to get something work */
statement:
              compound_statement
            | selection_statement
            | iteration_statement
            | expression ';'

selection_statement:
            /* IMPORTANT! not allowing assignments within IFs */
              IF_TOKEN '(' logical_or_expression ')' statement
            | IF_TOKEN '(' logical_or_expression ')' statement ELSE_TOKEN statement

iteration_statement:
              WHILE_TOKEN '(' logical_or_expression ')' statement

compound_statement:
              '{' block_item_list '}'
              {
                  $$ = $2;
              }
            ;
block_item_list:
              block_item
            | block_item_list block_item
            ;

block_item:
              declaration
            | statement
            ;


expression:       
              assignment_expression
            ;


logical_or_expression:
              logical_and_expression
            | logical_or_expression OR_TOKEN logical_and_expression

logical_and_expression:
              equality_expression 
            | logical_and_expression AND_TOKEN equality_expression
        
equality_expression:
              relational_expression
            | equality_expression EQUALITY_TOKEN relational_expression

relational_expression:
              additive_expression
            | relational_expression RELATIONAL_TOKEN additive_expression

additive_expression:
              multiplicative_expression
            | additive_expression '+' multiplicative_expression
              {
                  Instruction *instr = gen_additive_instr(symbol_table, $1, $3);

                  if (add_instr == NULL) 
                  { 
                      printf("INVALID ADD\n"); 
                  } 
                  else
                  {
                      add_instr(instr_list, &num_instrs, instr);
                  }

                    
                  Arg *arg = malloc(sizeof(Arg));
                  // Point instead to where the instr's result will be (it's temp symbol)
                  arg->type = IDENT;
                  arg->ident_val = instr->result;

                  /* I think this i bad.
                  arg->type = INSTR;
                  arg->instr_val=instr;
                  */
                  $$ = arg;
              }
            | additive_expression '-' multiplicative_expression
              {
                  Instruction *instr = gen_subtractive_instr(symbol_table, $1, $3);

                  if (add_instr == NULL) 
                  { 
                      printf("INVALID SUBTRACT\n"); 
                  } 
                  else
                  {
                      add_instr(instr_list, &num_instrs, instr);
                  }

                    
                  Arg *arg = malloc(sizeof(Arg));
                  // Point instead to where the instr's result will be (it's temp symbol)
                  arg->type = IDENT;
                  arg->ident_val = instr->result;

                  $$ = arg;
              }
            ;
            
multiplicative_expression:
              primary_expression
            | multiplicative_expression '*' primary_expression
              {
                  Instruction *mult_instr = gen_multiplicative_instr(symbol_table, $1, $3);

                  if (mult_instr == NULL) 
                  { 
                      printf("INVALID MULTIPLY\n"); 
                  } 
                  else
                  {
                      add_instr(instr_list, &num_instrs, mult_instr);
                  }

                  Arg *arg = malloc(sizeof(Arg));
                  // Point instead to where the instr's result will be (it's temp symbol)
                  arg->type = IDENT;
                  arg->ident_val = mult_instr->result;
                  $$ = arg;
              }
            ;

primary_expression:
              id 
              {
                  Arg *arg = malloc(sizeof(Arg));
                  arg->type = IDENT;
                  arg->ident_val = $1;
                  $$ = arg;
              }
            | literal
            | '(' expression ')'
              {
                  $$ = $2;
              }

assignment_expression:
              /* DEPRECATED with additions of conditionals additive_expression */
              logical_or_expression
              /* TODO: id should be unary-expression */
            | id ASSIGN_OP expression 
              {
                  Instruction *instr = init_instruction(ASSIGN, $3, NULL, $1);
                  add_instr(instr_list, &num_instrs, instr);
                  $$ = NULL;
              }
            ;

declaration:
                /* Clearly a subset as there are many more possible declerations */
              type_specifier IDENTIFIER ASSIGN_OP initializer ';'
              {
                  Identifier *id = get_identifier(symbol_table, $2);

                  if (id != NULL)
                  {
                      yyerror("Previously declared identifier");
                      YYERROR;
                  }

                  id = malloc(sizeof(Identifier));
                  id->type = $1;
                  id->symbol = $2;
                  // For now allocate everything to the stack
                  id->offset = stack_offset;
                  stack_offset += get_byte_size($1);

                  put_identifier(symbol_table, id);

                  Instruction *instr = init_instruction(ASSIGN, $4, NULL, id);
                  add_instr(instr_list, &num_instrs, instr);

                  $$ = NULL;
              }
              ;

/* For now, just allow initialization to a primary expression */
initializer:
            expression                
            ;

type_specifier: 
              INT_TYPE_TOKEN      { $$ = INTEGER; }
            | DOUBLE_TYPE_TOKEN   { $$ = DOUBLE;  }
            | LONG_TYPE_TOKEN     { $$ = LONG;    }
            | CHAR_TYPE_TOKEN     { $$ = CHAR;    }
            ;

literal:      INT_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = INTEGER;
                  cons->int_val = $1;

                  $$ = malloc(sizeof(Arg));
                  $$->type = CONST;
                  $$->const_val = cons;
              }
            | REAL_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = DOUBLE;
                  cons->int_val = $1;

                  $$ = malloc(sizeof(Arg));
                  $$->type = CONST;
                  $$->const_val = cons;
              }
            | LONG_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = LONG;
                  cons->long_val = $1;

                  $$ = malloc(sizeof(Arg));
                  $$->type = CONST;
                  $$->const_val = cons;
              }
            | CHAR_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = CHAR;
                  cons->str_val = $1;

                  $$ = malloc(sizeof(Arg));
                  $$->type = CONST;
                  $$->const_val = cons;
              }
            ;



id:         IDENTIFIER
            {
                // Attempts to grab symbol_id from table. If not there, return
                // an error (unitialized).

                $$ = get_identifier(symbol_table, $1);
                // We're just using one copy of the string.
                free($1);
                if ($$ == NULL)
                {
                    yyerror("Use of uninitialized value");
                    YYERROR;
                } 
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
    stack_offset = 0;

    yyparse();
 
    print_instr_list(instr_list, num_instrs);
    return 1;

    print_symbol_table(symbol_table);
    compile(instr_list, symbol_table, num_instrs, "inter.asm");
} 
