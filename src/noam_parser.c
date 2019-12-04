#include "noam_parser.h"

noam_token_info* noam_get_token_info(noam_parser* parser, int offset){
    return noam_buffer_at(parser->tokens, parser->index + offset);
}

int noam_match_token(noam_parser* parser, noam_token token){
    if(noam_get_token_info(parser, 0)->token != token)
        return 0;
    ++parser->index;
    return 1;
}

int noam_match_tokens(noam_parser* parser, noam_token first_token, noam_token second_token){
    if(noam_get_token_info(parser, 0)->token != first_token)
        return 0;
    if(noam_get_token_info(parser, 1)->token != second_token)
        return 0;
    parser->index += 2;
    return 1;
}

int noam_match_token_str(noam_parser* parser, const char* name){
    noam_token_info* info = noam_get_token_info(parser, 0);
    if(info->token != NOAM_WORD_TOKEN)
        return 0;
    if(strcmp(info->name->data, name) != 0)
        return 0;
    ++parser->index;
    return 1;
}

noam_token_info* noam_consume_token(noam_parser* parser, noam_token token){
    noam_token_info* info = noam_get_token_info(parser, 0);
    if(info->token != token)
        return NULL;
    ++parser->index;
    return info;
}

noam_token_info* noam_consume_token_str(noam_parser* parser, const char* name){
    if(!noam_match_token_str(parser, name))
        return NULL;
    return noam_get_token_info(parser, -1);
}

noam_buffer* noam_parse_func_args(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){
    noam_buffer* args = noam_buffer_createv(sizeof(noam_expression*), &noam_expression_release);

    if(!noam_match_token(parser, NOAM_RP_TOKEN)){
        noam_expression* arg = noam_parse_expression(parser, symbol_table, current_scope);
        noam_buffer_push(args, &arg);

        while(!noam_match_token(parser, NOAM_RP_TOKEN)){
            if(!noam_match_token(parser, NOAM_COMMA_TOKEN)){
                //TODO: Error
            }
            arg = noam_parse_expression(parser, symbol_table, current_scope);
            noam_buffer_push(args, &arg);
        }
    }

    return args;
}

noam_expression* noam_parse_atomic(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){
    if(noam_match_token(parser, NOAM_WORD_TOKEN)){
        noam_token_info* info = noam_get_token_info(parser, -1);
        if(noam_match_token(parser, NOAM_LP_TOKEN)){
            noam_buffer* args = noam_parse_func_args(parser, symbol_table, current_scope);
            return noam_func_call_expression_create(info->name, args, symbol_table);
        } else {
            return noam_variable_expression_create(info->name, current_scope);
        }
    } else if(noam_match_token(parser, NOAM_INT_TOKEN)){
        return noam_int_value_create(atoi(noam_get_token_info(parser, -1)->name->data));
    } else if(noam_match_token(parser, NOAM_FLOAT_TOKEN)){
        return noam_float_value_create(atof(noam_get_token_info(parser, -1)->name->data));
    } else if(noam_match_token(parser, NOAM_STRING_TOKEN)){
        return noam_string_value_create(noam_get_token_info(parser, -1)->name);
    } else if(noam_match_token(parser, NOAM_BOOL_TOKEN)){
        return noam_bool_value_create(noam_atob(noam_get_token_info(parser, -1)->name->data));
    } else if(noam_match_token(parser, NOAM_NIL_TOKEN)){
        return noam_nil_value_create();
    } else if(noam_match_token(parser, NOAM_LP_TOKEN)){
        noam_expression* expression = noam_parse_expression(parser, symbol_table, current_scope);

        if(!noam_match_token(parser, NOAM_RP_TOKEN)){
            //TODO: Error
        }

        return expression;
    }
    //TODO: Error
    return NULL;
}

noam_expression* noam_parse_op(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){
    noam_expression* lhs_expression = noam_parse_atomic(parser, symbol_table, current_scope);

    while(noam_match_token(parser, NOAM_OP_TOKEN)){
        noam_token_info* op = noam_get_token_info(parser, -1);
        noam_expression* rhs_expression = noam_parse_atomic(parser, symbol_table, current_scope);
        //TODO: Generalize op type to string
        return noam_op_expression_create(lhs_expression, op->name, rhs_expression);
    }

    return lhs_expression;
}

noam_expression* noam_parse_expression(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){
    return noam_parse_op(parser, symbol_table, current_scope);
}

void noam_parser_init(noam_parser* parser, const char* source){
    memset(parser, 0, sizeof(noam_parser));
    parser->tokens = noam_parse_tokens(source);

    /*for(size_t i = 0; i < parser->tokens->length; ++i){
        noam_token_info* info = noam_buffer_at(parser->tokens, i);
        printf("%s %s\n", (char*)info->name->data, noam_token_to_string(info->token));
    }*/
}

int noam_parser_end(noam_parser* parser){
    return parser->index < parser->tokens->length;
}


noam_cond_statement* noam_parse_cond(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* scope){
    noam_buffer* conds = noam_buffer_createv(sizeof(noam_expression*), &noam_expression_release);
    noam_buffer* blocks = noam_buffer_createv(sizeof(noam_buffer), &noam_buffer_release);
    noam_expression* cond = noam_parse_expression(parser, symbol_table, scope);

    noam_buffer_push(conds, &cond);

    if(!noam_match_token(parser, NOAM_LB_TOKEN)){
        //TODO: Error
    }

    noam_buffer* block = noam_parse_block(parser, symbol_table, scope);
    noam_buffer_push(blocks, block);
    //noam_buffer_release(block);

    if(!noam_match_token(parser, NOAM_RB_TOKEN)){
        //TODO: Error
    }

    int last_cond = 0;

    while(noam_match_token_str(parser, NOAM_ELSE_STR)){

        if(last_cond){
            //TODO: Error
        }

        if(noam_match_token_str(parser, NOAM_IF_STR)){
            last_cond = 0;
        } else {
            last_cond = 1;
        }

        if(!last_cond){
            cond = noam_parse_expression(parser, symbol_table, scope);
            noam_buffer_push(conds, &cond);
        }

        if(!noam_match_token(parser, NOAM_LB_TOKEN)){
            //TODO: Error
        }

        block = noam_parse_block(parser, symbol_table, scope);
        noam_buffer_push(blocks, block);

        //noam_buffer_release(block);

        if(!noam_match_token(parser, NOAM_RB_TOKEN)){
            //TODO: Error
        }
    }

    return noam_cond_statement_create(conds, blocks, last_cond);
}

noam_buffer* noam_parse_block(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){

    noam_buffer* statements = noam_buffer_create(sizeof(noam_statement*));

    while(noam_parser_end(parser)){

        while(noam_match_token(parser, NOAM_LINE_TOKEN));

        if(noam_match_tokens(parser, NOAM_WORD_TOKEN, NOAM_EQ_TOKEN)){
            noam_token_info* info = noam_get_token_info(parser, -2);
            noam_expression* expression = noam_parse_expression(parser, symbol_table, current_scope);
            void* assigment_statement = noam_assignment_statement_create(
                    info->name, expression, current_scope
            );
            noam_buffer_push(statements, &assigment_statement);
        } else if (noam_match_token_str(parser, NOAM_PRINT_STR)){
            void* print_statement = noam_print_statement_create(
                    noam_parse_expression(parser, symbol_table, current_scope)
            );
            noam_buffer_push(statements, &print_statement);
        } else if(noam_match_token_str(parser, NOAM_IF_STR)){
            void* cond_statement = noam_parse_cond(parser, symbol_table, current_scope);
            noam_buffer_push(statements, &cond_statement);
        } else if(noam_match_token(parser, NOAM_LB_TOKEN)) {
            noam_buffer* block = noam_parse_block(parser, symbol_table, noam_scope_add_child(NULL, current_scope));

            if (!noam_match_token(parser, NOAM_RB_TOKEN)) {
                //TODO: Error
            }

            if(!noam_buffer_empty(block)){
                noam_buffer_merge(statements, block);
            }

            //noam_buffer_release(block);
        } else if(noam_match_token_str(parser, NOAM_RETURN_STR)){
            noam_expression* expression = noam_parse_expression(parser, symbol_table, current_scope);
            void* return_statement = noam_return_statement_create(expression);
            noam_buffer_push(statements, &return_statement);
        } else {
            noam_expression* expression = noam_parse_expression(parser, symbol_table, current_scope);

            if(!expression){
                break;
            }

            void* statement = noam_expression_statement_create(expression);
            noam_buffer_push(statements, &statement);
        }

    }

    return statements;
}

void noam_parse_func(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope** scope){
    noam_token_info* func_name = noam_consume_token(parser, NOAM_WORD_TOKEN);

    if(!func_name){
        //TODO: Error
    }

    if(!noam_match_token(parser, NOAM_LP_TOKEN)){
        //TODO: Error
    }

    noam_buffer* params = noam_buffer_createv(sizeof(noam_buffer), &noam_buffer_release);

    if(!noam_match_token(parser, NOAM_RP_TOKEN)){
        noam_token_info* param = noam_consume_token(parser, NOAM_WORD_TOKEN);

        if(!param){
            //TODO: Error
        }

        noam_buffer_push(params, param->name);

        while(!noam_match_token(parser, NOAM_RP_TOKEN)){

            if(!noam_match_token(parser, NOAM_COMMA_TOKEN)){
                //TODO: Error
            }

            param = noam_consume_token(parser, NOAM_WORD_TOKEN);

            if(!param){
                //TODO: Error
            }

            noam_buffer_push(params, param->name);
        }
    }

    if(!noam_match_token(parser, NOAM_LB_TOKEN)){
        //TODO: Error
    }

    if(!*scope){
        *scope = noam_scope_add_child(func_name->name, symbol_table->head);
    } else {
        *scope = noam_scope_add_sibling(func_name->name, *scope);
    }

    //TODO: Check that releases
    noam_buffer* body = noam_buffer_createv(sizeof(noam_statement*), &noam_statement_release);

    while(!noam_match_token(parser, NOAM_RB_TOKEN)){
        noam_buffer* block = noam_parse_block(parser, symbol_table, *scope);

        if(!noam_buffer_empty(block)){
            noam_buffer_merge(body, block);
        }

        //noam_buffer_release(block);
    }

    noam_func* func = noam_func_create(func_name->name, params, body);
    noam_dict_insert(symbol_table->funcs, func_name->name, func);
}

noam_buffer* noam_parse_statements(noam_parser* parser, noam_symbol_table* symbol_table){
    noam_buffer* statements = noam_buffer_create(sizeof(noam_statement*));
    noam_scope* last_scope = NULL;

    while(noam_parser_end(parser)){
        if(noam_match_token_str(parser, NOAM_FUNC_STR)){
            noam_parse_func(parser, symbol_table, &last_scope);
        } else {
            noam_buffer* block = noam_parse_block(parser, symbol_table, symbol_table->head);
            if(noam_buffer_empty(block))
                continue;
            noam_buffer_merge(statements, block);
            //noam_buffer_release(block);
        }
    }

    return statements;
}