#include <ctype.h>

#include "noam_lexer.h"

noam_token_info* noam_token_info_create(noam_buffer* name, noam_token token){
    noam_token_info* info = malloc(sizeof(noam_token_info));
    info->name = noam_buffer_create(1);
    noam_buffer_merge(info->name, name);
    info->token = token;
    return info;
}

void noam_state_reset(noam_buffer* token_name, noam_state* state){
    noam_buffer_clear(token_name);
    *state = NOAM_DEFAULT_STATE;
}

void noam_token_info_release(noam_token_info* info){
    free(info->name);
}

noam_prefix_node* noam_prefix_node_create(char character){
    noam_prefix_node* node = malloc(sizeof(noam_prefix_node));
    memset(node, 0, sizeof(noam_prefix_node));
    node->character = character;
    node->token = NOAM_ERROR_TOKEN;
    return node;
}

noam_prefix_node* noam_prefix_node_add_child(noam_prefix_node* parent, char character){
    noam_prefix_node* node = noam_prefix_node_create(character);
    node->parent = parent;
    parent->child = node;
    return node;
}

noam_prefix_node* noam_prefix_node_add_sibling(noam_prefix_node* prev, char character){
    noam_prefix_node* node = noam_prefix_node_create(character);
    node->parent = prev->parent;
    prev->next = node;
    return node;
}

void noam_prefix_tree_insert(noam_prefix_node* root, const char* keyword, noam_token token){
    noam_prefix_node* parent = root;

    for(const char* k = keyword; *k != '\0'; ++k){
        noam_prefix_node* node = parent->child;

        if(!node){
            parent = noam_prefix_node_add_child(parent, *k);
        } else {
            while(node->character != *k && node->next){
                node = node->next;
            }

            if(node->character == *k){
                parent = node;
            } else {
                noam_prefix_node* k_node = noam_prefix_node_add_sibling(node, *k);
                parent = k_node;
            }
        }
    }

    parent->token = token;
}

int noam_prefix_tree_contains(noam_prefix_node* root, noam_buffer* keyword){
    noam_prefix_node* parent = root;

    for(size_t i = 0; i < keyword->length; ++i){
        char k = *(char*)noam_buffer_at(keyword, i);
        noam_prefix_node* node = parent->child;

        if(!node){
            return 0;
        }

        while(node->character != k && node->next){
            node = node->next;
        }

        if(node->character != k){
            return 0;
        }

        parent = node;
    }

    return 1;
}

noam_token noam_prefix_tree_find(noam_prefix_node* root, noam_buffer* keyword){
    noam_prefix_node* parent = root;

    for(size_t i = 0; i < keyword->length; ++i){
        char k = *(char*)noam_buffer_at(keyword, i);
        noam_prefix_node* node = parent->child;

        if(!node){
            return NOAM_ERROR_TOKEN;
        }

        while(node->character != k && node->next){
            node = node->next;
        }

        if(node->character != k){
            return NOAM_ERROR_TOKEN;
        }

        parent = node;
    }

    return parent->token;
}

noam_prefix_node* noam_prefix_tree_build(const char** keywords, const noam_token* tokens, size_t length){
    noam_prefix_node* root = noam_prefix_node_create(0);

    for(size_t i = 0; i < length; ++i){
        noam_prefix_tree_insert(root, keywords[i], tokens[i]);
    }

    return root;
}

noam_prefix_node* noam_tokens_tree(){
    static const char* tokens_str[] = { NOAM_TRUE_STR,
                                        NOAM_FALSE_STR,
                                        NOAM_EQ_STR,
                                        NOAM_PLUS_STR,
                                        NOAM_MINUS_STR,
                                        NOAM_MULT_STR,
                                        NOAM_DIV_STR,
                                        NOAM_LESS_STR,
                                        NOAM_GREATER_STR,
                                        NOAM_LP_STR,
                                        NOAM_RP_STR,
                                        NOAM_LB_STR,
                                        NOAM_RB_STR,
                                        NOAM_EQ2_STR,
                                        NOAM_NEQ_STR,
                                        NOAM_COMMA_STR,
                                        NOAM_NIL_STR };

    static const noam_token tokens[] = { NOAM_BOOL_TOKEN,
                                         NOAM_BOOL_TOKEN,
                                         NOAM_EQ_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_LP_TOKEN,
                                         NOAM_RP_TOKEN,
                                         NOAM_LB_TOKEN,
                                         NOAM_RB_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_OP_TOKEN,
                                         NOAM_COMMA_TOKEN,
                                         NOAM_NIL_TOKEN };

    return noam_prefix_tree_build(tokens_str, tokens, sizeof(tokens) / sizeof(noam_token));
}

noam_buffer* noam_parse_tokens(const char* source){
    noam_buffer* tokens = noam_buffer_createv(sizeof(noam_token_info), &noam_token_info_release);
    noam_state state = NOAM_DEFAULT_STATE;
    noam_buffer* token_name = noam_buffer_create(1);
    noam_prefix_node* tokens_root = noam_tokens_tree();

    for(const char* c = source; *c != '\0'; ++c){
        switch(state){
            case NOAM_DEFAULT_STATE: {
                if(isalpha(*c)){
                    state = NOAM_WORD_STATE;
                    noam_buffer_push(token_name, c);
                } else if(isdigit(*c)){
                    state = NOAM_NUMBER_STATE;
                    noam_buffer_push(token_name, c);
                } else if(*c == '"'){
                    state = NOAM_STRING_STATE;
                } else if(*c == '#') {
                    state = NOAM_COMMENT_STATE;
                } else if(*c == ' ' || *c == '\n'){
                    break;
                } else {
                    noam_buffer_push(token_name, c);

                    if(!noam_prefix_tree_contains(tokens_root, token_name)){
                        fprintf(stderr, "noam: unknown token");
                        exit(-1);
                    }

                    state = NOAM_SPEC_STATE;
                }
                break;
            }
            case NOAM_WORD_STATE: {
                if(isalpha(*c) || isdigit(*c)){
                    noam_buffer_push(token_name, c);
                } else {
                    noam_token token = noam_prefix_tree_find(tokens_root, token_name);

                    if(token != NOAM_ERROR_TOKEN){
                        noam_buffer_push(tokens, noam_token_info_create(token_name, token));
                    } else {
                        noam_buffer_push(tokens, noam_token_info_create(token_name, NOAM_WORD_TOKEN));
                    }

                    noam_state_reset(token_name, &state);
                    --c;
                }
                break;
            }
            case NOAM_NUMBER_STATE: { //TODO: zero case
                if(isdigit(*c)){
                    noam_buffer_push(token_name, c);
                } else if(*c == '.') {
                    noam_buffer_push(token_name, c);
                    state = NOAM_FRACTION_STATE;
                } else {
                    noam_buffer_push(tokens, noam_token_info_create(token_name, NOAM_INT_TOKEN));
                    noam_state_reset(token_name, &state);
                    --c;
                }
                break;
            }
            case NOAM_FRACTION_STATE: {
                if(isdigit(*c)){
                    noam_buffer_push(token_name, c);
                } else {
                    noam_buffer_push(tokens, noam_token_info_create(token_name, NOAM_FLOAT_TOKEN));
                    noam_state_reset(token_name, &state);
                    --c;
                }
                break;
            }
            case NOAM_STRING_STATE: {
                if(*c != '"'){
                    noam_buffer_push(token_name, c);
                } else {
                    noam_buffer_push(tokens, noam_token_info_create(token_name, NOAM_STRING_TOKEN));
                    noam_state_reset(token_name, &state);
                }
                break;
            }
            case NOAM_COMMENT_STATE: {
                if(*c == '\n'){
                    state = NOAM_DEFAULT_STATE;
                }
                break;
            }
            case NOAM_SPEC_STATE: {
                noam_buffer* temp = noam_buffer_copy(token_name);
                noam_token tf = noam_prefix_tree_find(tokens_root, token_name);
                noam_buffer_push(token_name, c);
                noam_token ts = noam_prefix_tree_find(tokens_root, token_name);

                if(ts != NOAM_ERROR_TOKEN){
                    noam_buffer_push(tokens, noam_token_info_create(token_name, ts));
                    noam_state_reset(token_name, &state);
                } else if(!noam_prefix_tree_contains(tokens_root, token_name)){
                    noam_buffer_push(tokens, noam_token_info_create(temp, tf));
                    noam_state_reset(token_name, &state);
                    --c;
                }

                noam_buffer_release(temp);
                break;
            }
        }
    }

    noam_buffer_release(token_name);
    return tokens;
}