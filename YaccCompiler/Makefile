all: y.tab.c lex.yy.c
	gcc y.tab.c lex.yy.c -o compileout

# Compile yacc file to C
# -d command specifies to create a header with token definitions (y.tab.h)
# Creates y.tab.c and y.tab.h
y.tab.c y.tab.h: C.y
	yacc -d C.y

# Compile lex regex's to C
# As it imports yacc_compile's results it needs to be recompiled
lex.yy.c: C.lex y.tab.h 
	flex C.lex

