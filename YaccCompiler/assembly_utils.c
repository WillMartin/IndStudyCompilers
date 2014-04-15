#include "assembly_utils.h"

FILE *open_file(char* filename)
{
    return fopen(filename, "w");
}

void close_file(FILE* f)
{
    fclose(f);
}

void write_out_instr(FILE* f, char* instr)
{
    if (f == NULL)
    {
        printf("ERROR WRITING TO FILE\n");
        exit(1);
    }

    fprintf(f, instr);
}
