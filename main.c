#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "syntactical.h"

extern char* input_text;

void free_token_list(Token *start) {
    Token *current = start;
    while (current != NULL) {
        Token *temp = current;
        current = current->next;
        // Free dynamically allocated memory for token text if necessary
        if (temp->code == ID || temp->code == CT_STRING) {
            free(temp->text);
        }
        free(temp); // Free memory allocated for the token
    }
}

void read_file(char *filename)
{
    printf("%s", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        err("Failed to open file %s", filename);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory dynamically
    input_text = malloc((file_size + 1) * sizeof(char));
    if (input_text == NULL) {
        fclose(file);
        err("Memory allocation failed");
    }
    char c;
    int n=0;

    while ((c = fgetc(file)) != EOF)
    {
        input_text[n++] = (char) c;
    }
    input_text[n] = '\0';

    fclose(file);
    
}

void cleanup(void) {
    free_token_list(tokens);
    free(input_text);
}

int main(int argc, char *argv[])
{
    read_file("0.c");

    atexit(cleanup);

    while(getNextToken()!=1);
    
    free(input_text);
    
    int sa_output = syntactical_analysis(tokens);
    printf("\nsyntactical analysis output: %d", sa_output);

    free_token_list(tokens);

    return 0;
}