CFLAGS=-std=c99 -g
WITH_GLIB=`pkg-config --cflags --libs glib-2.0`
OUT_FILE=compiler

all: y.tab.c lex.yy.c symbol_table.o inter_code_gen.o converter.o register.o
	gcc $(CFLAGS) y.tab.c lex.yy.c symbol_table.o inter_code_gen.o converter.o register.o -o $(OUT_FILE) $(WITH_GLIB)

# Compile yacc file to C
# -d command specifies to create a header with token definitions (y.tab.h)
# Creates y.tab.c and y.tab.h
y.tab.c y.tab.h: ScanAndParse/C.y symbol_table.o
	yacc -d ScanAndParse/C.y 

# Compile lex regex's to C
# As it imports yacc_compile's results it needs to be recompiled
lex.yy.c: ScanAndParse/C.lex y.tab.h symbol_table.o
	flex ScanAndParse/C.lex

symbol_table.o: InterCodeUtils/symbol_table.c InterCodeUtils/symbol_table.h
	gcc $(CFLAGS) -c InterCodeUtils/symbol_table.c $(WITH_GLIB)

inter_code_gen.o: InterCodeUtils/symbol_table.h InterCodeUtils/symbol_table.c InterCodeUtils/inter_code_gen.c InterCodeUtils/inter_code_gen.h
	gcc $(CFLAGS) -c InterCodeUtils/inter_code_gen.c InterCodeUtils/symbol_table.c $(WITH_GLIB)

converter.o: AssemblyBackend/converter.h AssemblyBackend/converter.c register.o
	gcc $(CFLAGS) -c AssemblyBackend/converter.h AssemblyBackend/converter.c register.o $(WITH_GLIB)

register.o: AssemblyBackend/register.h AssemblyBackend/register.c
	gcc $(CFLAGS) -c AssemblyBackend/register.h AssemblyBackend/register.c $(WITH_GLIB)

clean:
	rm -f *.o lex.yy.c y.tab.c t.tab.h
