#include <stdio.h>

#include "noam.h"

//TODO: Memory management awareness
// Release all the stuff

//TODO: For debugging
const char* noam_token_to_string(noam_token token){
    static const char* strings[] = { "NOAM_ERROR_TOKEN",
                                     "NOAM_WORD_TOKEN",
                                     "NOAM_INT_TOKEN",
                                     "NOAM_FLOAT_TOKEN",
                                     "NOAM_STRING_TOKEN",
                                     "NOAM_BOOL_TOKEN",
                                     "NOAM_LINE_TOKEN",
                                     "NOAM_EQ_TOKEN",
                                     "NOAM_OP_TOKEN",
                                     "NOAM_LP_TOKEN",
                                     "NOAM_RP_TOKEN",
                                     "NOAM_LB_TOKEN",
                                     "NOAM_RB_TOKEN",
                                     "NOAM_COMMA_TOKEN",
                                     "NOAM_NIL_TOKEN",
                                     "NOAM_EOF_TOKEN" };
    return strings[token];
}

void noam_interpret(noam_parser* parser, noam_symbol_table* symbol_table){
    noam_buffer* statements = noam_parse_statements(parser, symbol_table);
    noam_statements_run(statements);
    //TODO: Fix bug on statement release
    //noam_buffer_release(statements);
}

void noam_interactive_mode(noam_symbol_table* symbol_table){

    char* line = NULL;
    size_t length = 0;

    for(;;){
        printf(">>> ");
        getline(&line, &length, stdin);

        if(!strcmp(line, "exit()\n")){
            return;
        }

        noam_parser parser;
        noam_parser_init(&parser, line);
        noam_interpret(&parser, symbol_table);
    }
}

void noam_file_mode(const char* filename, noam_symbol_table* symbol_table){
    FILE* file = fopen(filename, "r");
    NOAM_EXIT(!file, "cannot open the file");

    const size_t chunk_size = 1024;
    char file_buffer[chunk_size] = {0};
    noam_buffer* file_source = noam_buffer_create(1);
    size_t bytes_read = 0;

    while((bytes_read = fread(file_buffer, 1, chunk_size, file)) > 0){
        noam_buffer_append(file_source, file_buffer, bytes_read);
    }

    //TODO: Consider replacing by noam_buffer_pushv
    char eof = ' ';
    noam_buffer_push(file_source, &eof);
    char* buffer = file_source->data;
    buffer[file_source->length + 1] = '\0';

    noam_parser parser;
    noam_parser_init(&parser, file_source->data);
    noam_interpret(&parser, symbol_table);

    fclose(file);
}

int main(int argc, char** argv) {
    NOAM_EXIT(argc != 2, "usage " NOAM_TITLE " [-i] [source]");

    noam_symbol_table* symbol_table = noam_symbol_table_create();

    if(!strcmp(argv[1], "-i")){
        noam_interactive_mode(symbol_table);
    } else {
        noam_file_mode(argv[1], symbol_table);
    }

    noam_symbol_table_release(symbol_table);

    return 0;
}