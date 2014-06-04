SHELL:=/bin/bash
CFLAGS=-std=c99 -g
WITH_GLIB=`pkg-config --cflags --libs glib-2.0`
OUT_FILE=compiler
INTER_FOLDER=InterCodeUtils

linux_program: all
	./compiler <<< "{int x= 0; while (x<20) {if (x > 10) { print(x); } x = x + 1;}}"
	nasm -f elf32 inter.asm 
	ld -melf_i386 -I/lib32/ld-linux.so.2 -lc -s -o program inter.o

mac_program: all
	./compiler <<< "{int x= 0; while (x<20) {if (x > 10) { print(x); } x = x + 1;}}"
	sed -i bak 's/_start/start/g' inter.asm
	sed -i bak 's/printf/_printf/g' inter.asm
	nasm -fmacho inter.asm
	ld -o program inter.o -lc

all: y.tab.c lex.yy.c symbol_table.o inter_code_gen.o converter.o register.o repr_utils.o optimization.o gc.o
	gcc $(CFLAGS) y.tab.c lex.yy.c symbol_table.o inter_code_gen.o converter.o register.o repr_utils.o gc.o optimization.o -o $(OUT_FILE) $(WITH_GLIB)

# Compile yacc file to C
# -d command specifies to create a header with token definitions (y.tab.h)
# Creates y.tab.c and y.tab.h
y.tab.c y.tab.h: ScanAndParse/C.y symbol_table.o
	yacc -d ScanAndParse/C.y 

# Compile lex regex's to C
# As it imports yacc_compile's results it needs to be recompiled
lex.yy.c: ScanAndParse/C.lex y.tab.h symbol_table.o
	flex ScanAndParse/C.lex

symbol_table.o: $(INTER_FOLDER)/symbol_table.c $(INTER_FOLDER)/symbol_table.h
	gcc $(CFLAGS) -c $(INTER_FOLDER)/symbol_table.c $(WITH_GLIB)

inter_code_gen.o: $(INTER_FOLDER)/symbol_table.h $(INTER_FOLDER)/symbol_table.c $(INTER_FOLDER)/inter_code_gen.c $(INTER_FOLDER)/inter_code_gen.h
	gcc $(CFLAGS) -c $(INTER_FOLDER)/inter_code_gen.c $(INTER_FOLDER)/symbol_table.c $(WITH_GLIB)

gc.o: $(INTER_FOLDER)/gc.h $(INTER_FOLDER)/gc.c $(INTER_FOLDER)/symbol_table.h $(INTER_FOLDER)/symbol_table.c $(INTER_FOLDER)/inter_code_gen.c $(INTER_FOLDER)/inter_code_gen.h
	gcc $(CFLAGS) -c $(INTER_FOLDER)/gc.c $(INTER_FOLDER)/symbol_table.c $(INTER_FOLDER)/inter_code_gen.c $(WITH_GLIB)

optimization.o: $(INTER_FOLDER)/optimization.h $(INTER_FOLDER)/optimization.c $(INTER_FOLDER)/symbol_table.h $(INTER_FOLDER)/symbol_table.c $(INTER_FOLDER)/inter_code_gen.c $(INTER_FOLDER)/inter_code_gen.h
	gcc $(CFLAGS) -c $(INTER_FOLDER)/optimization.c $(INTER_FOLDER)/symbol_table.c $(INTER_FOLDER)/inter_code_gen.c $(WITH_GLIB)

converter.o: AssemblyBackend/converter.h AssemblyBackend/converter.c register.o repr_utils.o
	gcc $(CFLAGS) -c AssemblyBackend/converter.h AssemblyBackend/converter.c register.o repr_utils.o $(WITH_GLIB)

register.o: AssemblyBackend/register.h AssemblyBackend/register.c
	gcc $(CFLAGS) -c AssemblyBackend/register.h AssemblyBackend/register.c $(WITH_GLIB)

repr_utils.o: AssemblyBackend/repr_utils.h AssemblyBackend/repr_utils.c 
	gcc $(CFLAGS) -c AssemblyBackend/repr_utils.h AssemblyBackend/repr_utils.c $(WITH_GLIB)

clean:
	rm -rf *.o lex.yy.c y.tab.c y.tab.h *.gch
