#!/bin/bash

nasm -f elf32 inter.asm 
#nasm -f elf64 inter.asm
#ld -melf_i386 -s -o program inter.o
ld -melf_i386 -I/lib32/ld-linux.so.2 -lc -s -o program inter.o
#ld -s -o program inter.o
