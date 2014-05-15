#!/bin/bash

nasm -f elf32 inter.asm 
#nasm -f elf64 inter.asm
ld -melf_i386 -s -o program inter.o
#ld -s -o program inter.o
