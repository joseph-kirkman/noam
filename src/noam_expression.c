#include "noam_expression.h"

#define NOAM_CHAR_BIT 8
#define NOAM_INT_CHAR_LENGTH ((NOAM_CHAR_BIT * sizeof(int) - 1) / 3 + 2)
#define NOAM_FLOAT_CHAR_LENGTH ((NOAM_CHAR_BIT * sizeof(float) - 1) / 3 + 2)

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
        fprintf(stderr, "noam: unknown variable %s", (const char*)expression->name->data);
        exit(-1);
        //TODO: Error
    }
    noam_expression** value = noam_dict_value(expression->scope->vars, node);
    return noam_expression_get(*value);
}

void noam_variable_expression_release(noam_variable_expression* expression){
    noam_buffer_release(expression->name);
}

noam_value* noam_func_call_expression_get(noam_func_call_expression* expression){
    noam_dict_node* node = noam_dict_find(expression->symbol_table->funcs, expression->name);

    if(!node){
        //TODO: Error
        fprintf(stderr, "unknown function %s", (const char*)expression->name->data);
        exit(-1);
    }

    noam_func* func = noam_dict_value(expression->symbol_table->funcs, node);

    //TODO: Find scope
    noam_scope* scope = expression->symbol_table->head->child;

    while(scope){
        if(noam_cmp_string(expression->name, scope->name) == 0){
            break;
        }
        scope = scope->next;
    }

    if(!scope){
        //TODO: Error
        fprintf(stderr, "cannot find a function scope");
        exit(-1);
    }

    if(expression->args->length != func->params->length){
        //TODO: Error
        fprintf(stderr, "function params mismatch: args=%lu params=%lu", expression->args->length, func->params->length);
        exit(-1);
    }

    for(size_t i = 0; i < expression->args->length; ++i){
        noam_buffer* param = noam_buffer_at(func->params, i);
        noam_expression** arg = noam_buffer_at(expression->args, i);
        noam_dict_insert(scope->vars, param, arg);
    }

#ifdef NOAM_DEBUG
    printf("%s(...) call\n", (char*)func->name->data);
#endif
    return noam_statements_run(func->body);
}

void noam_func_call_expression_release(noam_func_call_expression* expression){
    //TODO: Fix
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

noam_value* noam_bool_value_get(noam_bool_value* value){
    return value;
}

const char* noam_bool_value_to_string(noam_bool_value* value){
    return value->value == 1 ? NOAM_TRUE_STR : NOAM_FALSE_STR;
}

int noam_value_is_instance(const noam_value* value, noam_token type){
    return value->vtable_->type == type;
}

noam_variable_expression* noam_variable_expression_create(noam_buffer* name, noam_scope* scope){
    static noam_expression_vtable_ noam_variable_expression_vtable[] = {{&noam_variable_expression_get,
                                                                                &noam_variable_expression_release}};
#ifdef NOAM_DEBUG
    printf("noam_variable_expression_create: %s\n", (char*)name->data);
#endif
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
#ifdef NOAM_DEBUG
    printf("noam_func_call_expression_create: %s\n", (char*)name->data);
#endif
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
#ifdef NOAM_DEBUG
    printf("noam_int_value_create: %d\n", value);
#endif
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
#ifdef NOAM_DEBUG
    printf("noam_float_value_create: %f\n", value);
#endif
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
#ifdef NOAM_DEBUG
    printf("noam_string_value_create: %s\n", (char*)str->data);
#endif
    noam_string_value* string_value = malloc(sizeof(noam_string_value));
    string_value->vtable_ = noam_string_value_vtable;
    string_value->str = noam_buffer_create(1);
    noam_buffer_merge(string_value->str, str);
    return string_value;
}

noam_bool_value* noam_bool_value_create(int value){
    static noam_value_vtable_ noam_bool_value_vtable[] = {{&noam_bool_value_get,
                                                                  NULL,
                                                                  &noam_bool_value_to_string,
                                                                  NOAM_BOOL_TOKEN}};
#ifdef NOAM_DEBUG
    printf("noam_bool_value_create: %d\n", value);
#endif
    noam_bool_value* bool_value = malloc(sizeof(noam_bool_value));
    bool_value->vtable_ = noam_bool_value_vtable;
    bool_value->value = value;
    return bool_value;
}

int noam_values_equal_type(const noam_value* lhs, const noam_value* rhs, noam_token type){
    return noam_value_is_instance(lhs, type) && noam_value_is_instance(rhs, type);
}

noam_value* noam_op_expression_get(noam_op_expression* expression){
    //printf("op: %s\n", (char*)expression->op->data);
    noam_value* lhs = noam_expression_get(expression->lhs);
    noam_value* rhs = noam_expression_get(expression->rhs);

    //TODO: Type cast
    if(!strcmp(expression->op->data, NOAM_PLUS_STR)) {
        if (noam_values_equal_type(lhs, rhs, NOAM_FLOAT_TOKEN)){
            return noam_float_value_create(((noam_float_value*)lhs)->value + ((noam_float_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_INT_TOKEN)){
            return noam_int_value_create(((noam_int_value*)lhs)->value + ((noam_int_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_STRING_TOKEN)) {
            noam_buffer *str = noam_buffer_create(1);
            noam_buffer_merge(str, ((noam_string_value*)lhs)->str);
            noam_buffer_merge(str, ((noam_string_value*)rhs)->str);
            noam_string_value *value = noam_string_value_create(str);
            noam_buffer_release(str);
            return value;
        }

        //TODO: Error
    } else if(!strcmp(expression->op->data, NOAM_MINUS_STR)) {
        if (noam_values_equal_type(lhs, rhs, NOAM_FLOAT_TOKEN)){
            return noam_float_value_create(((noam_float_value*)lhs)->value - ((noam_float_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_INT_TOKEN)){
            return noam_int_value_create(((noam_int_value*)lhs)->value - ((noam_int_value*)rhs)->value);
        }
        //TODO: Error
    } else if(!strcmp(expression->op->data, NOAM_MULT_STR)) {
        if (noam_values_equal_type(lhs, rhs, NOAM_FLOAT_TOKEN)){
            return noam_float_value_create(((noam_float_value*)lhs)->value * ((noam_float_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_INT_TOKEN)){
            return noam_int_value_create(((noam_int_value*)lhs)->value * ((noam_int_value*)rhs)->value);
        }
        //TODO: Error
    } else if(!strcmp(expression->op->data, NOAM_DIV_STR)) {
        if (noam_values_equal_type(lhs, rhs, NOAM_FLOAT_TOKEN)){
            if(((noam_float_value*)rhs)->value == 0){
                //TODO: Error
            }
            return noam_float_value_create(((noam_float_value*)lhs)->value / ((noam_float_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_INT_TOKEN)){
            if(((noam_int_value*)rhs)->value == 0){
                //TODO: Error
            }
            return noam_int_value_create(((noam_int_value*)lhs)->value / ((noam_int_value*)rhs)->value);
        }
        //TODO: Error
    } else if(!strcmp(expression->op->data, NOAM_EQ2_STR)){
        if (noam_values_equal_type(lhs, rhs, NOAM_FLOAT_TOKEN)){
            return noam_bool_value_create(((noam_float_value*)lhs)->value == ((noam_float_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_INT_TOKEN)){
            return noam_bool_value_create(((noam_int_value*)lhs)->value == ((noam_int_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_BOOL_TOKEN)){
            return noam_bool_value_create(((noam_bool_value*)lhs)->value == ((noam_bool_value*)rhs)->value);
        }
        //TODO: Error
    } else if(!strcmp(expression->op->data, NOAM_NEQ_STR)){
        if (noam_values_equal_type(lhs, rhs, NOAM_FLOAT_TOKEN)){
            return noam_bool_value_create(((noam_float_value*)lhs)->value != ((noam_float_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_INT_TOKEN)){
            return noam_bool_value_create(((noam_int_value*)lhs)->value != ((noam_int_value*)rhs)->value);
        } else if (noam_values_equal_type(lhs, rhs, NOAM_BOOL_TOKEN)){
            return noam_bool_value_create(((noam_bool_value*)lhs)->value != ((noam_bool_value*)rhs)->value);
        }
        //TODO: Error
    }

    //TODO: Error
    return NULL;
}

void noam_op_expression_release(noam_op_expression* expression){
    //TODO
}

noam_op_expression* noam_op_expression_create(noam_expression* lhs, noam_buffer* op, noam_expression* rhs){
    static noam_expression_vtable_ noam_op_expression_vtable[] = {{&noam_op_expression_get,
                                                                          &noam_op_expression_release}};
#ifdef NOAM_DEBUG
    printf("noam_op_expression_create: %s\n", (char*)op->data);
#endif
    noam_op_expression* expression = malloc(sizeof(noam_op_expression));
    expression->vtable_ = noam_op_expression_vtable;
    expression->lhs = lhs;
    expression->op = op;
    expression->rhs = rhs;
    return expression;
}

noam_nil_value* noam_nil_value_create(){
    static noam_value_vtable_ noam_nil_value_vtable[] = {{&noam_nil_value_get,
                                                                   NULL,
                                                                   &noam_nil_value_to_string,
                                                                   NOAM_NIL_TOKEN}};
#ifdef NOAM_DEBUG
    printf("noam_nil_value_create\n");
#endif
    noam_nil_value* nil_value = malloc(sizeof(noam_nil_value));
    nil_value->vtable_ = noam_nil_value_vtable;
    return nil_value;
}

const char* noam_nil_value_to_string(noam_nil_value* value){
    return "nil";
}

noam_value* noam_nil_value_get(noam_nil_value* value){
    return value;
}