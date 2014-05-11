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
%type <ival> goto_place_holder
%type <argval> statement expression compound_statement assignment_expression block_item block_item_list declaration literal initializer additive_expression multiplicative_expression primary_expression logical_or_expression logical_and_expression equality_expression relational_expression selection_statement iteration_statement secondary_rel_expression
 
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

%token DOUBLE_TYPE_TOKEN INT_TYPE_TOKEN LONG_TYPE_TOKEN CHAR_TYPE_TOKEN BOOL_TYPE_TOKEN IF_TOKEN ELSE_TOKEN WHILE_TOKEN OR_TOKEN AND_TOKEN TRUE_TOKEN FALSE_TOKEN

%nonassoc LOWER_THAN_ELSE 
%nonassoc ELSE_TOKEN
/* Define operators and their precedence */

%%
 /* Rules */
statement:
              selection_statement
            | compound_statement
            | iteration_statement
            | expression ';'

selection_statement:
            /* IMPORTANT! not allowing assignments within IFs */
              IF_TOKEN '(' logical_or_expression ')' statement %prec LOWER_THAN_ELSE
              {
                  $$ = $3;
              }
            | IF_TOKEN '(' logical_or_expression ')' statement ELSE_TOKEN statement
              {
                  $$ = $3;
              }

iteration_statement:
              WHILE_TOKEN '(' logical_or_expression ')' statement
              {
                  $$ = $3;
              }

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

goto_place_holder:
            /* Just returns the current instr */
                {
                    $$ = num_instrs;
                }
            ;

logical_or_expression:
              logical_and_expression
            | logical_or_expression OR_TOKEN goto_place_holder logical_and_expression
              {
                  printf("Back patching! False List:");
                  print_list($1->false_list);
                  back_patch(instr_list, num_instrs, $1->false_list, $3);

                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = merge_lists($1->true_list, $4->true_list);
                  $$->false_list = $4->false_list;
              }

logical_and_expression:
              equality_expression 
            | logical_and_expression AND_TOKEN goto_place_holder equality_expression
              {
                  back_patch(instr_list, num_instrs, $1->true_list, $3);

                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = $4->true_list;
                  $$->false_list = merge_lists($1->false_list, $4->false_list);
              }
        
equality_expression:
              relational_expression
            | equality_expression EQUALITY_TOKEN relational_expression
              {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  /* Plus 1/2 because we need an operation then the gotos */
                  $$->true_list = make_list(num_instrs + 1);
                  $$->false_list = make_list(num_instrs + 2);

                  eOPCode op_code;
                  if (!strcmp($2, "==")) { op_code = EQ; }
                  else if (!strcmp($2, "!=")) { op_code = NEQ; }
                  else { assert(false); }

                  Identifier *temp = get_temp_symbol();
                  temp->type = INTEGER;
                  put_identifier(symbol_table, temp);

                  Instruction *op_instr = init_instruction(op_code, $1, $3, temp);
                  add_instr(instr_list, &num_instrs, op_instr);

                  Instruction *true_goto = init_instruction(GOTO, NULL, NULL, NULL);
                  add_instr(instr_list, &num_instrs, true_goto);
                  Instruction *false_goto = init_instruction(GOTO, NULL, NULL, NULL);
                  add_instr(instr_list, &num_instrs, false_goto);
              }

relational_expression:
                TRUE_TOKEN
                {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = make_list(num_instrs);
                  printf("In true, initing true list to %d\n", num_instrs);
                  Instruction *true_goto = init_instruction(GOTO, NULL, NULL, NULL);
                  add_instr(instr_list, &num_instrs, true_goto);
                }
              | FALSE_TOKEN
                {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->false_list = make_list(num_instrs);
                  Instruction *true_goto = init_instruction(GOTO, NULL, NULL, NULL);
                  add_instr(instr_list, &num_instrs, true_goto);
                }
              | secondary_rel_expression RELATIONAL_TOKEN additive_expression
              {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  /* Plus 1/2 because we need an operation then the gotos */
                  $$->true_list = make_list(num_instrs + 1);
                  $$->false_list = make_list(num_instrs + 2);

                  eOPCode op_code;
                  if (!strcmp($2, "<")) { op_code = LT; }
                  else if (!strcmp($2, ">")) { op_code = GT; }
                  else if (!strcmp($2, "<=")) { op_code = LEQ; }
                  else if (!strcmp($2, ">=")) { op_code = GEQ; }
                  else { assert(false); }

                  Identifier *temp = get_temp_symbol();
                  temp->type = INTEGER;
                  put_identifier(symbol_table, temp);

                  Instruction *op_instr = init_instruction(op_code, $1, $3, temp);
                  add_instr(instr_list, &num_instrs, op_instr);

                  Instruction *true_goto = init_instruction(GOTO, NULL, NULL, NULL);
                  add_instr(instr_list, &num_instrs, true_goto);
                  Instruction *false_goto = init_instruction(GOTO, NULL, NULL, NULL);
                  add_instr(instr_list, &num_instrs, false_goto);

              }

secondary_rel_expression:
              additive_expression
            | relational_expression


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

                    
                  // Point instead to where the instr's result will be (its temp symbol)
                  $$ = init_arg(IDENT, instr->result);
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

                    
                  // Point instead to where the instr's result will be (it's temp symbol)
                  $$ = init_arg(IDENT, instr->result);
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

                  // Point instead to where the instr's result will be (it's temp symbol)
                  $$ = init_arg(IDENT, mult_instr->result);
              }
            ;

primary_expression:
              id 
              {
                  $$ = init_arg(IDENT, $1);
              }
            | literal
            | '(' expression ')'
              {
                  $$ = $2;
              }

assignment_expression:
            /* Don't allow logicals and additives together */
              additive_expression
              /* TODO: id should be unary-expression */
            | logical_or_expression
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

                  // If it's a boolean we have to do some alternative work
                  if ($1 == BOOL)
                  {
                      Constant *true_const = malloc(sizeof(Constant));
                      true_const->type = BOOL;
                      true_const->bool_val = true;
                      Arg *true_arg = init_arg(CONST, true_const);

                      Constant *false_const = malloc(sizeof(Constant));
                      false_const->type = BOOL;
                      false_const->bool_val = false;
                      Arg *false_arg = init_arg(CONST, false_const);

                      // Make sure that initializer has something to backpatch
                      // If it doesn't then it's not a boolean expression
                      if ($4->false_list == NULL && $4->true_list == NULL)
                      {
                          yyerror("Invalid assignment to boolean assignment");
                          YYERROR;
                      }

                      // Generate two assigns. A True GOTO and a False assign
                      Instruction *true_assign = init_instruction(ASSIGN, 
                                                        true_arg, NULL, id);
                      Instruction *false_assign = init_instruction(ASSIGN, 
                                                        false_arg, NULL, id);
                      add_instr(instr_list, &num_instrs, true_assign);
                      add_instr(instr_list, &num_instrs, false_assign);
            
                      // Link all the true and false jumps correctly.
                      printf("BACK-PATCHING TRUE:");
                      print_list($4->true_list);
                      printf("BACK-PATCHING FALSE:");
                      print_list($4->false_list);
                      print_instr_list(instr_list, num_instrs);

                      back_patch(instr_list, num_instrs, $4->true_list, 
                                 num_instrs - 2);
                      back_patch(instr_list, num_instrs, $4->false_list, 
                                 num_instrs - 1);
                  }
                  else if ($1 != BOOL && $4->false_list == NULL && $4->true_list == NULL)
                  {
                      Instruction *instr = init_instruction(ASSIGN, $4, NULL, id);
                      add_instr(instr_list, &num_instrs, instr);
                  }
                  else
                  {
                      yyerror("Invalid assignment to non-boolean");
                      YYERROR;
                  }

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
            | BOOL_TYPE_TOKEN     { $$ = BOOL;    }
            ;

literal:      INT_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = INTEGER;
                  cons->int_val = $1;

                  $$ = init_arg(CONST, cons);
              }
            | REAL_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = DOUBLE;
                  cons->int_val = $1;

                  $$ = init_arg(CONST, cons);
              }
            | LONG_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = LONG;
                  cons->long_val = $1;

                  $$ = init_arg(CONST, cons);
              }
            | CHAR_LITERAL
              {
                  Constant *cons = malloc(sizeof(Constant));
                  cons->type = CHAR;
                  cons->str_val = $1;

                  $$ = init_arg(CONST, cons);
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
 
    printf("Finished Parsing\n");
    print_instr_list(instr_list, num_instrs);
    return 1;

    print_symbol_table(symbol_table);
    compile(instr_list, symbol_table, num_instrs, "inter.asm");
} 
