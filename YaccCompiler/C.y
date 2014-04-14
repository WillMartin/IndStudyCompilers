/* WDM Created 4/14/2014 */
%{
    /* Declarations: includes, globals */
    #include <stdio.h>
    /*#define YYSTYPE double * double type for Yacc stack (from book) */
%}

/* Define start symbol */
%start numlist

/*
/* Declare what types can be returned in yylval? /
%union { int int_val; 
         double dub_val; }
/* Define a type for non-terms (?) /
%type <int_val> NUMBER

/* Define tokens /
%token <int_val> DIGIT ID
%token <dub_val> DOUBLE  
*/
/* WORKING:  */
%token DIGIT 

/* Define operators and their precedence */

%%
 /* Rules */
numlist:                     /* empty */
            | numlist number /* Match two numbers in a row */
              {
                /* Rules are evaluated after recursive bit */
                printf("<new_digit:%d>", $2);
              }
            | number
              {
                /* Rules are evaluated after recursive bit */
                printf("<digit:%d>", $1);
              }
            ;
number: 
            DIGIT
            {
                /* Semantic actions for seeing a digit */
                $$ = $1 + $1;
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
