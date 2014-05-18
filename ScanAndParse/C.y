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
int yyerror(const char *str);
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
%type <ival> goto_place_holder end_if_jump
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
            /* IMPORTANT! not allowing assignments within IF conditionals */
            /* Also, not implementing the book's way of doing things here */
              IF_TOKEN '(' logical_or_expression ')' goto_place_holder compound_statement %prec LOWER_THAN_ELSE
              {
                  // If logical expr is true, enter if
                  back_patch(instr_list, num_instrs, $3->true_list, $5);
                  // If it's not go after compound_statement 
                  // Generate NOP for ease
                  Instruction *nop = init_nop_instr();
                  add_instr(instr_list, &num_instrs, nop);
                  // -1 to patch to the NOP we just added
                  back_patch(instr_list, num_instrs, $3->false_list, num_instrs - 1);

                  //$$ = init_arg(BOOLEAN_EXPR, NULL);
                  // TODO: figure out if we need to return anything here
                  $$ = NULL;
              }
              /*                    3                        5                 6              7                          9   */
            | IF_TOKEN '(' logical_or_expression ')' goto_place_holder compound_statement end_if_jump ELSE_TOKEN compound_statement
              {
                  // controls for initial jump
                  back_patch(instr_list, num_instrs, $3->true_list, $5);
                  // Add a NOP at the end of everything
                  Instruction *nop = init_nop_instr();
                  add_instr(instr_list, &num_instrs, nop);

                  GList *else_skip_list = make_list($7-1);

                  // Backpatch both jump over else and init jump to the end.
                  back_patch(instr_list, num_instrs, $3->false_list, $7);
                  back_patch(instr_list, num_instrs, else_skip_list, num_instrs-1);
              }

iteration_statement:
              /*  1            2             3            4            5        6             7          8 */
              WHILE_TOKEN goto_place_holder '(' logical_or_expression ')' goto_place_holder statement end_if_jump 
              {
                  back_patch(instr_list, num_instrs, $4->true_list, $6);
                  // And generate one NOP for when the loop terminates
                  Instruction *nop = init_nop_instr();
                  add_instr(instr_list, &num_instrs, nop);
                  back_patch(instr_list, num_instrs, $4->false_list, $8);

                  // Add force stack actions to make sure that there will always be 
                  // a current version that can be accessed for the while loop.
                  GList *actions = add_action_to_instr_range(instr_list, 
                                            num_instrs, $2, $6, FORCE_ID_STACK);
                  Instruction *lock_instr = get_instr(instr_list, num_instrs, $2);
                  lock_instr->actions = merge_lists(actions, lock_instr->actions);

                  // Don't forget to jump back to the top
                  GList *jump_list = make_list($8-1);
                  back_patch(instr_list, num_instrs, jump_list, $2);

                  // And then can free everything back up on the NOP
                  actions = add_action_to_instr_range(instr_list, num_instrs, 
                                                   $2, $6, RELEASE_ID_STACK);
                  nop->actions = actions;
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

end_if_jump:
             /* Returns the instr AFTER the newly made empty GOTO */
                {
                    // NULL until it is backpatched
                    Instruction *goto_instr = init_goto_instr(NULL);
                    add_instr(instr_list, &num_instrs, goto_instr);

                    $$ = num_instrs;
                }
            ;           

logical_or_expression:
              logical_and_expression
            | logical_or_expression OR_TOKEN goto_place_holder logical_and_expression
              {
                  back_patch(instr_list, num_instrs, $1->false_list, $3);

                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = merge_lists($1->true_list, $4->true_list);
                  $$->false_list = $4->false_list;
                  // Two arguments going into this are just a means to return
                  // true/false lits
                  free($1);
                  free($4);
              }

logical_and_expression:
              equality_expression 
            | logical_and_expression AND_TOKEN goto_place_holder equality_expression
              {
                  back_patch(instr_list, num_instrs, $1->true_list, $3);

                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = $4->true_list;
                  $$->false_list = merge_lists($1->false_list, $4->false_list);
                  // Two arguments going into this are just a means to return
                  // true/false lits
                  free($1);
                  free($4);
              }
        
equality_expression:
              relational_expression
            | equality_expression EQUALITY_TOKEN relational_expression
              {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  /* Cond operator and goto folllowed by the false GOTO */
                  $$->true_list = make_list(num_instrs);
                  $$->false_list = make_list(num_instrs + 1);

                  eOPCode op_code;
                  if (!strcmp($2, "==")) { op_code = EQ; }
                  else if (!strcmp($2, "!=")) { op_code = NEQ; }
                  else { assert(false); }

                  // NULL goto until it is backpatched
                  Instruction *cond_instr = init_cond_instr(op_code, $1, $3, NULL);
                  add_instr(instr_list, &num_instrs, cond_instr);

                  Instruction *false_goto = init_goto_instr(NULL);
                  add_instr(instr_list, &num_instrs, false_goto);

                  // Two arguments going into this are just a means to return
                  // true/false lits
                  free($1);
                  free($3);
              }

relational_expression:
                TRUE_TOKEN
                {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = make_list(num_instrs);
                  Instruction *true_goto = init_goto_instr(NULL);
                  add_instr(instr_list, &num_instrs, true_goto);
                }
              | FALSE_TOKEN
                {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->false_list = make_list(num_instrs);
                  Instruction *false_goto = init_goto_instr(NULL);
                  add_instr(instr_list, &num_instrs, false_goto);
                }
              | secondary_rel_expression RELATIONAL_TOKEN additive_expression
              {
                  $$ = init_arg(BOOLEAN_EXPR, NULL);
                  $$->true_list = make_list(num_instrs);
                  $$->false_list = make_list(num_instrs + 1);

                  eOPCode op_code;
                  if (!strcmp($2, "<")) { op_code = LT; }
                  else if (!strcmp($2, ">")) { op_code = GT; }
                  else if (!strcmp($2, "<=")) { op_code = LEQ; }
                  else if (!strcmp($2, ">=")) { op_code = GEQ; }
                  else { assert(false); }
                  free($2);

                  // NULL goto until it is backpatched
                  Instruction *cond_instr = init_cond_instr(op_code, $1, $3, NULL);
                  add_instr(instr_list, &num_instrs, cond_instr);

                  Instruction *false_goto = init_goto_instr(NULL);
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
                  Instruction *instr = init_assign_instr($3, $1);
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
                  id->on_stack = false;
                  id->force_on_stack = false;
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

                      // If it doesn't then it's not a boolean expression
                      if ($4->false_list == NULL && $4->true_list == NULL)
                      {
                          yyerror("Invalid assignment to boolean assignment");
                          YYERROR;
                      }

                      Instruction *true_assign = init_assign_instr(true_arg, id);
                      // Need to link interm goto to it. It NEEDS to be added last
                      Instruction *nop = init_nop_instr();
                      Instruction *interm_goto = init_goto_instr(nop);
                      Instruction *false_assign = init_assign_instr(false_arg, id);

                      add_instr(instr_list, &num_instrs, true_assign);
                      add_instr(instr_list, &num_instrs, interm_goto);
                      add_instr(instr_list, &num_instrs, false_assign);
                      add_instr(instr_list, &num_instrs, nop);
            
                      back_patch(instr_list, num_instrs, $4->true_list, 
                                 num_instrs - 3);
                      back_patch(instr_list, num_instrs, $4->false_list, 
                                 num_instrs - 1);
                      g_list_free($4->true_list);
                      g_list_free($4->false_list);
                      free($4);
                  }
                  else if ($1 != BOOL && $4->false_list == NULL && $4->true_list == NULL)
                  {
                      Instruction *instr = init_assign_instr($4, id);
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
 
    print_instr_list(instr_list, num_instrs);

    print_symbol_table(symbol_table);
    compile(instr_list, symbol_table, num_instrs, "inter.asm");
} 
