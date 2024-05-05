#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "syntactical.h"

extern char* input_text;
Token* tokens;

int main()
{
    read_file("1.c");
    tokens = lexical_analysis();
    int sa_output = syntactical_analysis(tokens);
    printf("\nsyntactical analysis output: %d", sa_output);

    return 0;
}