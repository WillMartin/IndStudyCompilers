all: y.tab.c lex.yy.c symbol_table.o
	gcc y.tab.c lex.yy.c symbol_table.o -o compileout `pkg-config --cflags --libs glib-2.0`

# Compile yacc file to C
# -d command specifies to create a header with token definitions (y.tab.h)
# Creates y.tab.c and y.tab.h
y.tab.c y.tab.h: C.y assembly_utils.o symbol_table.o
	yacc -d C.y 

# Compile lex regex's to C
# As it imports yacc_compile's results it needs to be recompiled
lex.yy.c: C.lex y.tab.h symbol_table.o
	flex C.lex

# Compile all assembly_utils
assembly_utils.o: assembly_utils.c assembly_utils.h
	gcc -c assembly_utils.c

symbol_table.o: symbol_table.c symbol_table.h
	gcc -c symbol_table.c `pkg-config --cflags --libs glib-2.0`

clean:
	rm -f *.o lex.yy.c y.tab.c t.tab.h
