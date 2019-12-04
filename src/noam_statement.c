#include "noam_statement.h"

noam_value* noam_statement_run(noam_statement* statement){
    return statement->vtable_->run(statement);
}

void noam_statement_release(noam_statement* statement){
    if(statement->vtable_->release){
        statement->vtable_->release(statement);
    }
}

noam_value* noam_print_statement_run(noam_print_statement* statement){
    noam_value* value = noam_expression_get(statement->expr);
    printf("%s\n", noam_value_to_string(value));
    return value;
}

void noam_print_statement_release(noam_print_statement* statement){
    noam_expression_release(statement->expr);
}

noam_print_statement* noam_print_statement_create(noam_expression* expression){
    static noam_statement_vtable_ noam_print_statement_vtable[] = {{&noam_print_statement_run,
                                                                    &noam_print_statement_is_returned,
                                                                    &noam_print_statement_release}};
#ifdef NOAM_DEBUG
    printf("noam_print_statement_create\n");
#endif
    noam_print_statement* statement = malloc(sizeof(noam_print_statement));
    statement->vtable_ = noam_print_statement_vtable;
    statement->expr = expression;
    return statement;
}

noam_value* noam_assignment_statement_run(noam_assignment_statement* statement){
    noam_dict_insert(statement->scope->vars, statement->name, &statement->expr);
    return noam_expression_get(statement->expr);
}

void noam_assignment_statement_release(noam_assignment_statement* statement){
    noam_expression_release(statement->expr);
    noam_buffer_release(statement->name);
}

noam_value* noam_expression_statement_run(noam_expression_statement* statement){
    return noam_expression_get(statement->expression);
}

void noam_expression_statement_release(noam_expression_statement* statement){
    noam_expression_release(statement->expression);
}

noam_value* noam_return_statement_run(noam_return_statement* statement){
    return noam_expression_get(statement->expression);
}

void noam_return_statement_release(noam_return_statement* statement){
    noam_expression_release(statement->expression);
}

noam_value* noam_cond_statement_run(noam_cond_statement* statement){
    size_t i = 0;

    while(i < statement->conditions->length){
        noam_expression** expression = noam_buffer_at(statement->conditions, i);
        noam_value* value = noam_expression_get(*expression);

        if(value->vtable_->type != NOAM_BOOL_TOKEN){
            //TODO: Error
            fprintf(stderr, "noam: condition is not a boolean type");
            exit(-1);
        }

        noam_bool_value* cond = (noam_bool_value*)value;

        if(cond->value){
            noam_buffer* block = noam_buffer_at(statement->blocks, i);
            noam_value* result = noam_statements_run(block);

            if(result){
                statement->is_returned = 1;
                return result;
            }
        }

        ++i;
    }

    if(statement->with_else){
        noam_buffer* block = noam_buffer_last(statement->blocks);
        noam_value* result = noam_statements_run(block);

        if(result){
            statement->is_returned = 1;
            return result;
        }
    }

    return NULL;
}

void noam_cond_statement_release(noam_cond_statement* statement){
    //TODO
}

noam_assignment_statement* noam_assignment_statement_create(noam_buffer* name,
                                                            noam_expression* expression,
                                                            noam_scope* scope){
    static noam_statement_vtable_ noam_assignment_statement_vtable[] = {{&noam_assignment_statement_run,
                                                                         &noam_assignment_statement_is_returned,
                                                                         &noam_assignment_statement_release}};
#ifdef NOAM_DEBUG
    printf("noam_assignment_statement_create: %s\n", (char*)name->data);
#endif
    noam_assignment_statement* statement = malloc(sizeof(noam_assignment_statement));
    statement->vtable_ = noam_assignment_statement_vtable;
    statement->name = name;
    statement->expr = expression;
    statement->scope = scope;
    return statement;
}

noam_expression_statement* noam_expression_statement_create(noam_expression* expression){
    static noam_statement_vtable_ noam_expression_statement_vtable[] = {{&noam_expression_statement_run,
                                                                         &noam_expression_statement_is_returned,
                                                                         &noam_expression_statement_release}};
#ifdef NOAM_DEBUG
    printf("noam_expression_statement_create\n");
#endif
    noam_expression_statement* statement = malloc(sizeof(noam_expression_statement));
    statement->vtable_ = noam_expression_statement_vtable;
    statement->expression = expression;
    return statement;
}

noam_return_statement* noam_return_statement_create(noam_expression* expression){
    static noam_statement_vtable_ noam_return_statement_vtable[] = {{&noam_return_statement_run,
                                                                     &noam_return_statement_is_returned,
                                                                     &noam_return_statement_release}};
#ifdef NOAM_DEBUG
    printf("noam_return_statement_create\n");
#endif
    noam_return_statement* statement = malloc(sizeof(noam_return_statement));
    statement->vtable_ = noam_return_statement_vtable;
    statement->expression = expression;
    return statement;
}

noam_cond_statement* noam_cond_statement_create(noam_buffer* conditions, noam_buffer* blocks, int with_else){
    static noam_statement_vtable_ noam_cond_statement_vtable[] = {{&noam_cond_statement_run,
                                                                  &noam_cond_statement_is_returned,
                                                                  &noam_cond_statement_release}};
#ifdef NOAM_DEBUG
    printf("noam_cond_statement_create\n");
#endif
    noam_cond_statement* statement = malloc(sizeof(noam_cond_statement));
    statement->vtable_ = noam_cond_statement_vtable;
    statement->conditions = conditions;
    statement->blocks = blocks;
    statement->with_else = with_else;
    statement->is_returned = 0;
    return statement;
}

noam_value* noam_statements_run(noam_buffer* statements){
    size_t current = 0;
#ifdef NOAM_DEBUG
    printf("noam_statements_run\n");
#endif
    while(current < statements->length){
        noam_statement** statement = noam_buffer_at(statements, current++);
        noam_value* result = noam_statement_run(*statement);

        if(noam_statement_is_returned(*statement)){
            return result;
        }
    }

    return NULL;
}

int noam_statement_is_returned(noam_statement* statement){
    return statement->vtable_->is_returned(statement);
}

int noam_print_statement_is_returned(noam_print_statement* statement){
    return 0;
}

int noam_assignment_statement_is_returned(noam_assignment_statement* statement){
    return 0;
}

int noam_expression_statement_is_returned(noam_expression_statement* statement){
    return 0;
}

int noam_return_statement_is_returned(noam_return_statement* statement){
    return 1;
}

int noam_cond_statement_is_returned(noam_cond_statement* statement){
    return statement->is_returned;
}