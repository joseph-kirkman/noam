#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define NOAM_TOKEN_MAX_LENGTH 256
#define NOAM_BUFFER_GROW_FACTOR 2
#define NOAM_CHAR_BIT 8
#define NOAM_INT_CHAR_LENGTH ((NOAM_CHAR_BIT * sizeof(int) - 1) / 3 + 2)
#define NOAM_FLOAT_CHAR_LENGTH ((NOAM_CHAR_BIT * sizeof(float) - 1) / 3 + 2)

//TODO: Memory management awareness
// Release all the stuff

typedef enum {
    NOAM_DEFAULT_STATE,
    NOAM_WORD_STATE,
    NOAM_STRING_STATE,
    NOAM_NUMBER_STATE,
    NOAM_FRACTION_STATE,
    NOAM_COMMENT_STATE,
} noam_state;

typedef enum {
    NOAM_ERROR_TOKEN,
    NOAM_WORD_TOKEN,
    NOAM_INT_TOKEN,
    NOAM_FLOAT_TOKEN,
    NOAM_STRING_TOKEN,
    NOAM_LINE_TOKEN,
    NOAM_EQ_TOKEN,
    NOAM_OP_TOKEN,
    NOAM_LP_TOKEN,
    NOAM_RP_TOKEN,
    NOAM_LB_TOKEN,
    NOAM_RB_TOKEN,
    NOAM_EOF_TOKEN
} noam_token;

#define NOAM_IF_TOKEN "if"
#define NOAM_PRINT_TOKEN "print"
#define NOAM_FUNC_TOKEN "func"

typedef void(*noam_release_func)(void*);

typedef struct {
    void*             data;
    size_t            length;
    size_t            size;
    size_t            chunk;
    noam_release_func release;
} noam_buffer;

typedef struct noam_dict_node {
    char*                  data;
    struct noam_dict_node* next;
} noam_dict_node;

typedef size_t(*noam_hash_func)(const void*);
typedef int(*noam_cmp_func)(const void*, const void*);

typedef struct {
    noam_dict_node*   nodes;
    size_t            length;
    size_t            size;
    size_t            key_chunk;
    size_t            val_chunk;
    noam_hash_func    hash;
    noam_cmp_func     cmp;
    noam_release_func release;
} noam_dict;


typedef struct {
    noam_buffer* name;
    noam_token   token;
} noam_token_info;

typedef struct {
    noam_buffer* tokens;
    size_t       index;
} noam_parser;

typedef struct noam_scope {
    noam_dict*         vars;
    struct noam_scope* parent;
    struct noam_scope* next;
    struct noam_scope* child;
} noam_scope;

typedef struct {
    noam_buffer* name;
    noam_buffer* params;
    noam_buffer* body;
} noam_func;

typedef struct {
    noam_dict* funcs;
    noam_scope* head;
} noam_symbol_table;

struct noam_statement;
struct noam_value;
struct noam_expression;

typedef void(*noam_statement_run_func)(struct noam_statement*);
typedef struct noam_value*(*noam_expression_get_func)(struct noam_expression*);
typedef const char*(*noam_value_to_string_func)(struct noam_value*);

typedef struct {
    noam_expression_get_func get;
    noam_release_func        release;
} noam_expression_vtable_;

typedef struct {
    noam_expression_get_func  get;
    noam_release_func         release;
    noam_value_to_string_func to_string;
    noam_token                type;
} noam_value_vtable_;

typedef struct noam_expression {
    noam_expression_vtable_* vtable_;
} noam_expression;

typedef struct {
    noam_expression_vtable_* vtable_;
    noam_buffer*             name;
    noam_scope*              scope;
} noam_variable_expression;

typedef struct {
    noam_expression_vtable_* vtable_;
    noam_buffer*             name;
    noam_buffer*             args;
    noam_symbol_table*       symbol_table;
} noam_func_call_expression;

typedef struct noam_value {
    noam_value_vtable_* vtable_;
} noam_value;

typedef struct {
    noam_value_vtable_* vtable_;
    int                 value;
} noam_int_value;

typedef struct {
    noam_value_vtable_* vtable_;
    float               value;
} noam_float_value;

typedef struct {
    noam_value_vtable_* vtable_;
    noam_buffer*        str;
} noam_string_value;

typedef struct {
    noam_expression_vtable_* vtable_;
    char                     op;
    noam_expression*         lhs;
    noam_expression*         rhs;
} noam_op_expression;

typedef struct {
    noam_statement_run_func run;
    noam_release_func       release;
} noam_statement_vtable_;

typedef struct noam_statement {
    const noam_statement_vtable_* vtable_;
} noam_statement;

typedef struct {
    noam_statement_vtable_* vtable_;
    struct noam_expression* expr;
} noam_print_statement;

typedef struct {
    noam_statement_vtable_* vtable_;
    noam_buffer*            name;
    struct noam_expression* expr;
    noam_scope*             scope;
} noam_assignment_statement;

typedef struct {
    noam_statement_vtable_*    vtable_;
    noam_func_call_expression* func_call;
} noam_func_call_statement;

noam_buffer* noam_buffer_createv(size_t chunk, noam_release_func release){
    noam_buffer* buffer = malloc(sizeof(noam_buffer));
    buffer->data = malloc(chunk);
    memset(buffer->data, 0, chunk);
    buffer->length = 0;
    buffer->size = 1;
    buffer->chunk = chunk;
    buffer->release = release;
    return buffer;
}

noam_buffer* noam_buffer_create(size_t chunk){
    return noam_buffer_createv(chunk, NULL);
}

void* noam_buffer_at(noam_buffer* buffer, size_t index){
    return (unsigned char*)buffer->data + index * buffer->chunk;
}

void* noam_buffer_first(noam_buffer* buffer){
    return buffer->data;
}

void* noam_buffer_last(noam_buffer* buffer){
    return noam_buffer_at(buffer, buffer->length - 1);
}

void noam_buffer_release(noam_buffer* buffer){
    if(buffer->release){
        for(size_t i = 0; i < buffer->length; ++i){
            buffer->release(noam_buffer_at(buffer, i));
        }
    }
    free(buffer->data);
    free(buffer);
}

void noam_buffer_grow(noam_buffer* buffer, size_t size){
    buffer->data = realloc(buffer->data, size * buffer->chunk);
    buffer->size = size;
}

void noam_buffer_push(noam_buffer* buffer, void* data){
    if(buffer->length >= buffer->size){
        noam_buffer_grow(buffer, NOAM_BUFFER_GROW_FACTOR * buffer->size);
    }
    memmove(noam_buffer_at(buffer, buffer->length), data, buffer->chunk);
    ++buffer->length;
}

void noam_buffer_assign(noam_buffer* buffer, void* data, size_t length){
    if(buffer->length + length >= buffer->size){
        noam_buffer_grow(buffer, NOAM_BUFFER_GROW_FACTOR * (buffer->length + length));
    }
    memmove(noam_buffer_at(buffer, buffer->length), data, length * buffer->chunk);
    buffer->length += length;
}

void noam_buffer_merge(noam_buffer* buffer, noam_buffer* other){
    noam_buffer_assign(buffer, other->data, other->length);
}

int noam_buffer_empty(noam_buffer* buffer){
    return buffer->length == 0;
}

void noam_buffer_clear(noam_buffer* buffer){

    if(buffer->release){
        for(size_t i = 0; i < buffer->length; ++i){
            buffer->release(noam_buffer_at(buffer, i));
        }
    }

    buffer->data = realloc(buffer->data, buffer->chunk);
    buffer->length = 0;
    buffer->size = 1;
}

noam_dict* noam_dict_createv(size_t key_chunk, size_t val_chunk,
                             noam_hash_func hash, noam_cmp_func cmp,
                             noam_release_func release){
    noam_dict* dict = malloc(sizeof(noam_dict));
    dict->nodes = malloc(sizeof(noam_dict_node));
    memset(dict->nodes, 0, sizeof(noam_dict_node));
    dict->key_chunk = key_chunk;
    dict->val_chunk = val_chunk;
    dict->length = 0;
    dict->size = 1;
    dict->hash = hash;
    dict->cmp = cmp;
    dict->release = release;
    return dict;
}

noam_dict* noam_dict_create(size_t key_chunk, size_t val_chunk, noam_hash_func hash){
    return noam_dict_createv(key_chunk, val_chunk, hash, NULL, NULL);
}

noam_dict_node* noam_dict_node_create(noam_dict* dict, void* key, void* value){
    noam_dict_node* node = malloc(sizeof(node));
    node->data = malloc(dict->key_chunk + dict->val_chunk);
    memcpy(node->data, key, dict->key_chunk);
    memcpy(node->data + dict->key_chunk, value, dict->val_chunk);
    node->next = NULL;
    return node;
}

void noam_dict_insert(noam_dict* dict, void* key, void* value){
    //TODO: Rehash on purpose
    size_t hash = dict->hash(key) % dict->size;
    noam_dict_node* node = noam_dict_node_create(dict, key, value);
    noam_dict_node* head = &dict->nodes[hash];

    while(head->next){
        head = head->next;
    }

    head->next = node;
}

noam_dict_node* noam_dict_find(noam_dict* dict, void* key){
    size_t hash = dict->hash(key) % dict->size;
    noam_dict_node* node = dict->nodes[hash].next;

    if(dict->cmp){
        while(node){
            if(dict->cmp(node->data, key) == 0)
                return node;
            node = node->next;
        }
        return NULL;
    } else {
        while(node){
            if(memcmp(node->data, key, dict->key_chunk) == 0)
                return node;
            node = node->next;
        }
        return NULL;
    }
}

void* noam_dict_value(noam_dict* dict, noam_dict_node* node){
    return node->data + dict->key_chunk;
}

void noam_dict_release(noam_dict* dict){
    if(dict->release){
        for(size_t i = 0; i < dict->size; ++i){
            noam_dict_node* node = dict->nodes[i].next;

            while(node){
                noam_dict_node* next = node->next;
                dict->release(node->data);
                free(node);
                node = next;
            }
        }
    }

    free(dict->nodes);
    free(dict);
}

noam_token noam_char_to_token(char c){
    static const char* tokens_char = "\n=+-*/<>(){}";
    static const noam_token tokens_type[] = { NOAM_LINE_TOKEN, NOAM_EQ_TOKEN, NOAM_OP_TOKEN,
                                              NOAM_OP_TOKEN, NOAM_OP_TOKEN, NOAM_OP_TOKEN, NOAM_OP_TOKEN,
                                              NOAM_OP_TOKEN, NOAM_LP_TOKEN, NOAM_RP_TOKEN,
                                              NOAM_LB_TOKEN, NOAM_RB_TOKEN };

    for(const char* token = tokens_char; *token != '\0'; ++token){
        if(*token == c)
            return tokens_type[token - tokens_char];
    }

    return NOAM_ERROR_TOKEN;
}

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

noam_buffer* noam_parse_tokens(const char* source){
    noam_buffer* tokens = noam_buffer_createv(sizeof(noam_token_info), &noam_token_info_release);
    noam_state state = NOAM_DEFAULT_STATE;
    noam_buffer* token_name = noam_buffer_create(1);

    for(const char* c = source; *c != '\0'; ++c){
        switch(state){
            case NOAM_DEFAULT_STATE: {
                noam_token token = noam_char_to_token(*c);
                if(token != NOAM_ERROR_TOKEN){
                    noam_buffer_push(token_name, c);
                    noam_buffer_push(tokens, noam_token_info_create(token_name, token));
                    noam_buffer_clear(token_name);
                } else if(isalpha(*c)){
                    state = NOAM_WORD_STATE;
                    noam_buffer_push(token_name, c);
                } else if(isdigit(*c)){
                    state = NOAM_NUMBER_STATE;
                    noam_buffer_push(token_name, c);
                } else if(*c == '"'){
                    state = NOAM_STRING_STATE;
                } else if(*c == '#') {
                    state = NOAM_COMMENT_STATE;
                }
                break;
            }
            case NOAM_WORD_STATE: {
                if(isalpha(*c) || isdigit(*c)){
                    noam_buffer_push(token_name, c);
                } else {
                    noam_buffer_push(tokens, noam_token_info_create(token_name, NOAM_WORD_TOKEN));
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
        }
    }

    noam_buffer_release(token_name);
    return tokens;
}

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

noam_value* noam_expression_get(noam_expression* expression){
    return expression->vtable_->get(expression);
}

const char* noam_value_to_string(noam_value* value){
    return value->vtable_->to_string(value);
}

void noam_expression_release(noam_expression* expression){
    if(expression->vtable_->release){
        expression->vtable_->release(expression);
    }
}

noam_value* noam_variable_expression_get(noam_variable_expression* expression){
    noam_scope* scope = expression->scope;
    noam_dict_node* node = NULL;

    while(scope){
        if((node = noam_dict_find(scope->vars, expression->name)))
            break;
        scope = scope->parent;
    }

    if(!node){
        fprintf(stderr, "noam: unknown variable");
        exit(-1);
        //TODO: Error
    }
    noam_expression** value = noam_dict_value(expression->scope->vars, node);
    return noam_expression_get(*value);
}

void noam_variable_expression_release(noam_variable_expression* expression){
    noam_buffer_release(expression->name);
}

void noam_statement_run(noam_statement* statement);

noam_value* noam_func_call_expression_get(noam_func_call_expression* expression){
    noam_dict_node* node = noam_dict_find(expression->symbol_table->funcs, expression->name);

    if(!node){
        //TODO: Error
        fprintf(stderr, "unknown function");
        exit(-1);
    }

    noam_func* func = noam_dict_value(expression->symbol_table->funcs, node);

    //TODO: Find scope
    noam_scope* scope = expression->symbol_table->head->child;

    if(expression->args->length != func->params->length){
        //TODO: Error
        fprintf(stderr, "param num mismatch");
        exit(-1);
    }

    for(size_t i = 0; i < expression->args->length; ++i){
        noam_buffer* param = noam_buffer_at(func->params, i);
        noam_buffer* arg = noam_buffer_at(expression->args, i);
        noam_dict_insert(scope->vars, param, arg);
    }

    for(size_t i = 0; i < func->body->length; ++i){
        noam_statement** statement = noam_buffer_at(func->body, i);
        noam_statement_run(*statement);
    }

    return 0;
}

void noam_func_call_expression_release(noam_func_call_expression* expression){
    noam_buffer_release(expression->name);
    noam_buffer_release(expression->args);
}

noam_value* noam_int_value_get(noam_int_value* value){
    return value;
}

const char* noam_int_value_to_string(noam_int_value* value){
    static char str[NOAM_INT_CHAR_LENGTH];
    sprintf(str, "%d", value->value);
    return str;
}

noam_value* noam_float_value_get(noam_float_value* value){
    return value;
}

const char* noam_float_value_to_string(noam_float_value* value){
    static char str[NOAM_FLOAT_CHAR_LENGTH];
    sprintf(str, "%f", value->value);
    return str;
}

noam_value* noam_string_value_get(noam_string_value* value){
    return value;
}

void noam_string_value_release(noam_string_value* value){
    noam_buffer_release(value->str);
}

const char* noam_string_value_to_string(noam_string_value* value){
    return value->str->data;
}

int noam_value_is_instance(noam_value* value, noam_token type){
    return value->vtable_->type == type;
}

noam_variable_expression* noam_variable_expression_create(noam_buffer* name, noam_scope* scope){
    static noam_expression_vtable_ noam_variable_expression_vtable[] = {{&noam_variable_expression_get,
                                                                         &noam_variable_expression_release}};
    noam_variable_expression* expression = malloc(sizeof(noam_variable_expression));
    expression->vtable_ = noam_variable_expression_vtable;
    expression->name = name;
    expression->scope = scope;
    return expression;
}

noam_func_call_expression* noam_func_call_expression_create(
        noam_buffer* name, noam_buffer* args, noam_symbol_table* symbol_table){
    static noam_expression_vtable_ noam_func_call_expression_vtable[] = {{&noam_func_call_expression_get,
                                                                          &noam_func_call_expression_release}};
    noam_func_call_expression* expression = malloc(sizeof(noam_func_call_expression));
    expression->vtable_ = noam_func_call_expression_vtable;
    expression->name = name;
    expression->args = args;
    expression->symbol_table = symbol_table;
    return expression;
}

noam_int_value* noam_int_value_create(int value){
    static noam_value_vtable_ noam_int_value_vtable[] = {{&noam_int_value_get,
                                                          NULL,
                                                          &noam_int_value_to_string,
                                                          NOAM_INT_TOKEN}};
    noam_int_value* int_value = malloc(sizeof(noam_int_value));
    int_value->vtable_ = noam_int_value_vtable;
    int_value->value = value;
    return int_value;
}

noam_float_value* noam_float_value_create(float value){
    static noam_value_vtable_ noam_float_value_vtable[] = {{&noam_float_value_get,
                                                            NULL,
                                                            &noam_float_value_to_string,
                                                            NOAM_FLOAT_TOKEN}};
    noam_float_value* float_value = malloc(sizeof(noam_float_value));
    float_value->vtable_ = noam_float_value_vtable;
    float_value->value = value;
    return float_value;
}

noam_string_value* noam_string_value_create(noam_buffer* str){
    static noam_value_vtable_ noam_string_value_vtable[] = {{&noam_string_value_get,
                                                             &noam_string_value_release,
                                                             &noam_string_value_to_string,
                                                             NOAM_STRING_TOKEN}};
    noam_string_value* string_value = malloc(sizeof(noam_string_value));
    string_value->vtable_ = noam_string_value_vtable;
    string_value->str = noam_buffer_create(1);
    noam_buffer_merge(string_value->str, str);
    return string_value;
}

int noam_convertible_to_float(noam_value* lhs, noam_value* rhs){
    return (noam_value_is_instance(lhs, NOAM_INT_TOKEN) && noam_value_is_instance(rhs, NOAM_FLOAT_TOKEN)) ||
           (noam_value_is_instance(lhs, NOAM_FLOAT_TOKEN) && noam_value_is_instance(rhs, NOAM_INT_TOKEN));
}

int noam_convertible_to_int(noam_value* lhs, noam_value* rhs){
    return (noam_value_is_instance(lhs, NOAM_INT_TOKEN) && noam_value_is_instance(rhs, NOAM_INT_TOKEN));
}

int noam_convertible_to_string(noam_value* lhs, noam_value* rhs){
    return (noam_value_is_instance(lhs, NOAM_STRING_TOKEN) && noam_value_is_instance(rhs, NOAM_STRING_TOKEN));
}

noam_value* noam_op_expression_get(noam_op_expression* expression){
    noam_value* lhs = noam_expression_get(expression->lhs);
    noam_value* rhs = noam_expression_get(expression->rhs);

    switch(expression->op){
        case '+': {
            if(noam_convertible_to_float(lhs, rhs)){
                return noam_float_value_create(((noam_float_value*)lhs)->value + ((noam_float_value*)rhs)->value);
            } else if(noam_convertible_to_int(lhs, rhs)){
                return noam_int_value_create(((noam_int_value*)lhs)->value + ((noam_int_value*)rhs)->value);
            } else if(noam_convertible_to_string(lhs, rhs)){
                noam_buffer* str = noam_buffer_create(1);
                noam_buffer_merge(str, ((noam_string_value*)lhs)->str);
                noam_buffer_merge(str, ((noam_string_value*)rhs)->str);
                noam_string_value* value = noam_string_value_create(str);
                noam_buffer_release(str);
                return value;
            }

            //TODO: Error
        }
        case '-': {
            if(noam_convertible_to_float(lhs, rhs)){
                return noam_float_value_create(((noam_float_value*)lhs)->value - ((noam_float_value*)rhs)->value);
            } else if(noam_convertible_to_int(lhs, rhs)){
                return noam_int_value_create(((noam_int_value*)lhs)->value - ((noam_int_value*)rhs)->value);
            }
            //TODO: Error
        }
        case '*': {
            if(noam_convertible_to_float(lhs, rhs)){
                return noam_float_value_create(((noam_float_value*)lhs)->value * ((noam_float_value*)rhs)->value);
            } else if(noam_convertible_to_int(lhs, rhs)){
                return noam_int_value_create(((noam_int_value*)lhs)->value * ((noam_int_value*)rhs)->value);
            }
            //TODO: Error
        }
        case '/': {
            if(noam_convertible_to_float(lhs, rhs) || noam_convertible_to_int(lhs, rhs)){
                if(((noam_float_value*)rhs)->value == 0){
                    //TODO: Error
                }
                return noam_float_value_create(((noam_float_value*)lhs)->value / ((noam_float_value*)rhs)->value);
            }
            //TODO: Error
        }
        default:
            return NULL;
    }
}

noam_op_expression* noam_op_expression_create(noam_expression* lhs, char op, noam_expression* rhs){
    static noam_expression_vtable_ noam_op_expression_vtable[] = {{&noam_op_expression_get}};
    noam_op_expression* expression = malloc(sizeof(noam_op_expression));
    expression->vtable_ = noam_op_expression_vtable;
    expression->lhs = lhs;
    expression->op = op;
    expression->rhs = rhs;
    return expression;
}

noam_expression* noam_parse_expression(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope);

noam_buffer* noam_parse_func_args(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){
    noam_buffer* args = noam_buffer_createv(sizeof(noam_expression*), &noam_expression_release);

    if(!noam_match_token(parser, NOAM_RP_TOKEN)){
        noam_expression* arg = noam_parse_expression(parser, symbol_table, current_scope);
        noam_buffer_push(args, &arg);

        while(!noam_match_token(parser, NOAM_RP_TOKEN)){
            if(!noam_match_token_str(parser, ",")){
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
        noam_token_info* info = noam_get_token_info(parser, -1);
        return noam_string_value_create(info->name);
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
        return noam_op_expression_create(lhs_expression, *(char*)noam_buffer_at(op->name, 0), rhs_expression);
    }

    return lhs_expression;
}

noam_expression* noam_parse_expression(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* current_scope){
    return noam_parse_op(parser, symbol_table, current_scope);
}

void noam_statement_run(noam_statement* statement){
    statement->vtable_->run(statement);
}

void noam_statement_release(noam_statement* statement){
    if(statement->vtable_->release){
        statement->vtable_->release(statement);
    }
}

void noam_print_statement_run(noam_print_statement* statement){
    noam_value* value = noam_expression_get(statement->expr);
    printf("%s\n", noam_value_to_string(value));
}

void noam_print_statement_release(noam_print_statement* statement){
    noam_expression_release(statement->expr);
}

noam_print_statement* noam_print_statement_create(noam_expression* expression){
    static noam_statement_vtable_ noam_print_statement_vtable[] = {{&noam_print_statement_run,
                                                                    &noam_print_statement_release}};
    noam_print_statement* statement = malloc(sizeof(noam_print_statement));
    statement->vtable_ = noam_print_statement_vtable;
    statement->expr = expression;
    return statement;
}

void noam_assignment_statement_run(noam_assignment_statement* statement){
    noam_dict_insert(statement->scope->vars, statement->name, &statement->expr);
}

void noam_assignment_statement_release(noam_assignment_statement* statement){
    noam_expression_release(statement->expr);
    noam_buffer_release(statement->name);
}

void noam_func_call_statement_run(noam_func_call_statement* statement){
    //TODO: Call func
    noam_expression_get(statement->func_call);
}

void noam_func_call_statement_release(noam_func_call_statement* statement){
    noam_expression_release(statement->func_call);
}

noam_assignment_statement* noam_assignment_statement_create(noam_buffer* name,
                                                            noam_expression* expression,
                                                            noam_scope* scope){
    static noam_statement_vtable_ noam_assignment_statement_vtable[] = {{&noam_assignment_statement_run,
                                                                         &noam_assignment_statement_release}};
    noam_assignment_statement* statement = malloc(sizeof(noam_assignment_statement));
    statement->vtable_ = noam_assignment_statement_vtable;
    statement->name = name;
    statement->expr = expression;
    statement->scope = scope;
    return statement;
}

noam_func_call_statement* noam_func_call_statement_create(noam_func_call_expression* func_call){
    static noam_statement_vtable_ noam_func_call_statement_vtable[] = {{&noam_func_call_statement_run,
                                                                        &noam_func_call_statement_release}};
    noam_func_call_statement* statement = malloc(sizeof(noam_func_call_statement));
    statement->vtable_ = noam_func_call_statement_vtable;
    statement->func_call = func_call;
    return statement;
}

size_t noam_hash_string(const noam_buffer* str){
    size_t hash = 5381;
    const char* s = str->data;

    while(*s){
        hash = ((hash << 5) + hash) + *(s++);
    }

    return hash;
}

int noam_cmp_string(const noam_buffer* lhs, const noam_buffer* rhs){
    return strcmp(lhs->data, rhs->data);
}

void noam_scope_vars_release(void* data){
    noam_buffer_release(data);
    noam_expression_release(*(noam_expression**)((char*)data + sizeof(noam_buffer)));
}

noam_scope* noam_scope_create(){
    noam_scope* scope = malloc(sizeof(noam_scope));
    memset(scope, 0, sizeof(noam_scope));
    scope->vars = noam_dict_createv(sizeof(noam_buffer), sizeof(noam_value*),
                                    &noam_hash_string, &noam_cmp_string,
                                    &noam_scope_vars_release);
    return scope;
}

noam_scope* noam_scope_add_child(noam_scope* parent){
    noam_scope* scope = noam_scope_create();
    parent->child = scope;
    scope->parent = parent;
    scope->next = NULL;
    scope->child = NULL;
    return scope;
}

noam_scope* noam_scope_add_sibling(noam_scope* prev){
    noam_scope* scope = noam_scope_create();
    prev->next = scope;
    scope->parent = prev->parent;
    scope->next = NULL;
    scope->child = NULL;
    return scope;
}

void noam_scope_release(noam_scope* head){
    while(head){
        noam_scope* node = head;
        noam_dict_release(head->vars);
        free(head);
        head = node->next;
    }
}

noam_func* noam_func_create(noam_buffer* name, noam_buffer* params, noam_buffer* body){
    noam_func* func = malloc(sizeof(noam_func));
    func->name = name;
    func->params = params;
    func->body = body;
    return func;
}

void noam_func_release(noam_func* func){
    noam_buffer_release(func->name);
    noam_buffer_release(func->params);
    //TODO: Statement release throws right now
    //noam_buffer_release(func->body);
}

void noam_symbol_table_funcs_release(void* data){
    noam_buffer_release(data);
    noam_func_release((noam_func*)((char*)data + sizeof(noam_buffer)));
}

noam_symbol_table* noam_symbol_table_create(){
    noam_symbol_table* symbol_table = malloc(sizeof(noam_symbol_table));
    symbol_table->head = noam_scope_create();
    symbol_table->funcs = noam_dict_createv(sizeof(noam_buffer), sizeof(noam_func),
                                            &noam_hash_string, &noam_cmp_string,
                                            &noam_symbol_table_funcs_release);
    return symbol_table;
}

void noam_symbol_table_release(noam_symbol_table* symbol_table){
    noam_dict_release(symbol_table->funcs);
    //TODO: Traverse tree
    //noam_scope_release(symbol_table->head);
}

int noam_parser_end(noam_parser* parser){
    return parser->index < parser->tokens->length;
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
        } else if (noam_match_token_str(parser, NOAM_PRINT_TOKEN)){
            void* print_statement = noam_print_statement_create(
                    noam_parse_expression(parser, symbol_table, current_scope)
            );
            noam_buffer_push(statements, &print_statement);
        } else if(noam_match_token_str(parser, NOAM_IF_TOKEN)){
            noam_expression* expression = noam_parse_expression(parser, symbol_table, current_scope);
            //noam_consume_token(parser, NOAM_LB_TOKEN);

            //noam_consume_token(parser, NOAM_RB_TOKEN);
        } else if(noam_match_tokens(parser, NOAM_WORD_TOKEN, NOAM_LP_TOKEN)){
            noam_token_info* info = noam_get_token_info(parser, -2);
            noam_buffer* args = noam_parse_func_args(parser, symbol_table, current_scope);
            void* func_call_statement = noam_func_call_statement_create(
                    noam_func_call_expression_create(info->name, args, symbol_table)
            );
            noam_buffer_push(statements, &func_call_statement);
        } else if(noam_match_token(parser, NOAM_LB_TOKEN)) {
            noam_buffer* block = noam_parse_block(parser, symbol_table, noam_scope_add_child(current_scope));

            if (!noam_match_token(parser, NOAM_RB_TOKEN)) {
                //TODO: Error
            }

            if(!noam_buffer_empty(block)){
                noam_buffer_merge(statements, block);
            }

            noam_buffer_release(block);
        } else {
            break;
        }

    }

    return statements;
}

void noam_parse_func(noam_parser* parser, noam_symbol_table* symbol_table, noam_scope* scope){
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

            if(!noam_match_token_str(parser, ",")){
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

    noam_scope* func_scope = NULL;

    if(!scope){
        func_scope = noam_scope_add_child(symbol_table->head);
    } else {
        func_scope = noam_scope_add_sibling(scope);
    }

    //TODO: Check that releases
    noam_buffer* body = noam_buffer_createv(sizeof(noam_statement*), &noam_statement_release);

    while(!noam_match_token(parser, NOAM_RB_TOKEN)){
        noam_buffer* block = noam_parse_block(parser, symbol_table, func_scope);

        if(!noam_buffer_empty(block)){
            noam_buffer_merge(body, block);
        }

        noam_buffer_release(block);
    }

    noam_func* func = noam_func_create(func_name->name, params, body);
    noam_dict_insert(symbol_table->funcs, func_name->name, func);
}

noam_buffer* noam_parse_statements(noam_parser* parser, noam_symbol_table* symbol_table){
    noam_buffer* statements = noam_buffer_createv(sizeof(noam_statement*), &noam_statement_release);
    noam_scope* last_scope = NULL;

    while(noam_parser_end(parser)){
        if(noam_match_token_str(parser, NOAM_FUNC_TOKEN)){
            noam_parse_func(parser, symbol_table, last_scope);
        } else {
            noam_buffer* block = noam_parse_block(parser, symbol_table, symbol_table->head);
            if(noam_buffer_empty(block))
                break;
            noam_buffer_merge(statements, block);
            noam_buffer_release(block);
        }
    }

    return statements;
}

int main(int argc, char** argv) {
    if(argc != 2){
        fprintf(stderr, "usage: noam <source>\n");
        exit(-1);
    }
    FILE* file = fopen(argv[1], "r");

    if(!file){
        fprintf(stderr, "noam: cannot open the file");
        exit(-1);
    }

    const size_t chunk_size = 1024;
    char file_buffer[chunk_size] = {0};
    noam_buffer* file_source = noam_buffer_create(1);
    size_t bytes_read = 0;

    while((bytes_read = fread(file_buffer, 1, chunk_size, file)) > 0){
        noam_buffer_assign(file_source, file_buffer, bytes_read);
    }

    //TODO: Consider replacing by noam_buffer_pushv
    const char* file_source_eof = " ";
    noam_buffer_push(file_source, file_source_eof);

    fclose(file);

    noam_parser parser = {0};
    parser.tokens = noam_parse_tokens(file_source->data);

    noam_symbol_table* symbol_table = noam_symbol_table_create();
    noam_buffer* statements = noam_parse_statements(&parser, symbol_table);
    size_t current = 0;

    while(current < statements->length){
        noam_statement** statement = noam_buffer_at(statements, current++);
        noam_statement_run(*statement);
    }

    noam_symbol_table_release(symbol_table);
    noam_buffer_release(file_source);
    noam_buffer_release(parser.tokens);

    //TODO: Fix bug on statement release
    //noam_buffer_release(statements);

    return 0;
}