nasm -f elf32 inter.asm 
ld -melf_i386 -s -o assem_compiler inter.o
