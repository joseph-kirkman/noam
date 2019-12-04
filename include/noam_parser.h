#ifndef NOAM_PARSER_H
#define NOAM_PARSER_H

#include "noam_statement.h"

/* noam_parser struct: iterates over tokens and preserves the state of parsing
 *
 * tokens: array of noam_token_info
 * index: position of the currently parsing token */
typedef struct {
    noam_buffer* tokens;
    size_t       index;
} noam_parser;

noam_token_info* noam_get_token_info(noam_parser* parser, int offset);
int noam_match_token(noam_parser* parser, noam_token token);
int noam_match_tokens(noam_parser* parser, noam_token first_token, noam_token second_token);
int noam_match_token_str(noam_parser* parser, const char* name);
noam_token_info* noam_consume_token(noam_parser* parser, noam_token token);
noam_token_info* noam_consume_token_str(noam_parser* parser, const char* name);
noam_buffer* noam_parse_func_args(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope);
noam_expression* noam_parse_atomic(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope);
noam_expression* noam_parse_op(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope);
noam_expression* noam_parse_expression(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope);
noam_cond_statement* noam_parse_cond(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* scope);
noam_buffer* noam_parse_block(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope);
void noam_parse_func(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope** scope);
noam_buffer* noam_parse_statements(noam_parser* parser, noam_symbol_table* symbol_table);

void noam_parser_init(noam_parser* parser, const char* source);
int noam_parser_end(noam_parser* parser);

#endif //NOAM_PARSER_H
