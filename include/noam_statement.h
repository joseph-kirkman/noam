#ifndef NOAM_STATEMENT_H
#define NOAM_STATEMENT_H

#include "noam_expression.h"

struct noam_statement;

/* noam_statement_run_func: called when a statement is processed */
typedef noam_value*(*noam_statement_run_func)(struct noam_statement*);

/* noam_statement_is_returned: if a statement returns */
typedef int*(*noam_statement_is_returned_func)(struct noam_statement*);

typedef struct {
    noam_statement_run_func         run;
    noam_statement_is_returned_func is_returned;
    noam_release_func               release;
} noam_statement_vtable_;

/* noam_statement struct: a base for all runnable statements */
typedef struct noam_statement {
    const noam_statement_vtable_* vtable_;
} noam_statement;

/* noam_print_statement struct: evaluates an expression and prints it to the stdout
 *
 * expr: expression to be evaluated */
typedef struct {
    noam_statement_vtable_* vtable_;
    struct noam_expression* expr;
} noam_print_statement;

/* noam_assignment_statement struct: variable assignment
 *
 * name: name of the variable
 * expr: right hand side expression
 * scope: scope for variable pushing
 *
 * once a statement is run variable and its value is pushed to the scope
 * */
typedef struct {
    noam_statement_vtable_* vtable_;
    noam_buffer*            name;
    struct noam_expression* expr;
    noam_scope*             scope;
} noam_assignment_statement;

/* noam_expression_statement struct: an expression that discards its value
 *
 * func_call: an expression to be called, returned value is escaped
 * */
typedef struct {
    noam_statement_vtable_*    vtable_;
    noam_expression *          expression;
} noam_expression_statement;

/* noam_return_statement struct: returns from the function or main program
 *
 * expression: an expression to be evaluated on return */
typedef struct {
    noam_statement_vtable_* vtable_;
    noam_expression*        expression;
} noam_return_statement;

/* noam_cond_statement struct: an if/else statement */
typedef struct {
    noam_statement_vtable_* vtable_;
    noam_buffer*            conditions;
    noam_buffer*            blocks;
    int                     with_else;
    int                     is_returned;
} noam_cond_statement;

noam_value* noam_statement_run(noam_statement* statement);
int noam_statement_is_returned(noam_statement* statement);
void noam_statement_release(noam_statement* statement);

noam_print_statement* noam_print_statement_create(noam_expression* expression);
noam_value* noam_print_statement_run(noam_print_statement* statement);
int noam_print_statement_is_returned(noam_print_statement* statement);
void noam_print_statement_release(noam_print_statement* statement);

noam_assignment_statement* noam_assignment_statement_create(noam_buffer* name,
                                                            noam_expression* expression,
                                                            noam_scope* scope);
noam_value* noam_assignment_statement_run(noam_assignment_statement* statement);
int noam_assignment_statement_is_returned(noam_assignment_statement* statement);
void noam_assignment_statement_release(noam_assignment_statement* statement);

noam_expression_statement* noam_expression_statement_create(noam_expression* expression);
noam_value* noam_expression_statement_run(noam_expression_statement* statement);
int noam_expression_statement_is_returned(noam_expression_statement* statement);
void noam_expression_statement_release(noam_expression_statement* statement);

noam_return_statement* noam_return_statement_create(noam_expression* expression);
noam_value* noam_return_statement_run(noam_return_statement* statement);
int noam_return_statement_is_returned(noam_return_statement* statement);
void noam_return_statement_release(noam_return_statement* statement);

noam_cond_statement* noam_cond_statement_create(noam_buffer* conditions, noam_buffer* blocks, int with_else);
noam_value* noam_cond_statement_run(noam_cond_statement* statement);
int noam_cond_statement_is_returned(noam_cond_statement* statement);
void noam_cond_statement_release(noam_cond_statement* statement);

noam_value* noam_statements_run(noam_buffer* statements);

#endif //NOAM_STATEMENT_H
