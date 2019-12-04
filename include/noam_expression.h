#ifndef NOAM_EXPRESSION_H
#define NOAM_EXPRESSION_H

#include "noam_lexer.h"
#include "noam_symbol.h"

struct noam_value;
struct noam_expression;

/* noam_expression_get_func: evaluates an expression to a specific value */
typedef struct noam_value*(*noam_expression_get_func)(struct noam_expression*);

/* noam_value_to_string_func: converts a value to string */
typedef const char*(*noam_value_to_string_func)(struct noam_value*);

/* these vtables emulate virtual functions calls, because C doesn't support polymorphism by default
 * in order to be compliant with a specific call, object must have a pointer to a proper virtual table
 * with all function pointers passed
 *
 * some kind of an inheritance chain:
 * noam_expression -> noam_value -> noam_int_value, ...
 * noam_statement -> noam_print_statement, ...
 * */
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

/* noam_expression struct: is a base for all expressions
 *
 * referencing a variable, simple types, function call, op expressions, etc */
typedef struct noam_expression {
    noam_expression_vtable_* vtable_;
} noam_expression;

/* noam_variable_expression struct: variable referencing
 *
 * name: name of the variable
 * scope: scope in which it is defined
 * */
typedef struct {
    noam_expression_vtable_* vtable_;
    noam_buffer*             name;
    noam_scope*              scope;
} noam_variable_expression;

/* noam_func_call_expression struct: function call
 *
 * name: name of the function
 * args: arguments passed to function as noam_expressions array
 * symbol_table: needed for function lookup and scope finding
 * */
typedef struct {
    noam_expression_vtable_* vtable_;
    noam_buffer*             name;
    noam_buffer*             args;
    noam_symbol_table*       symbol_table;
} noam_func_call_expression;

/* noam_value struct: in most cases a value of some internal type
 *
 * int, float, string, bool */
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
    noam_value_vtable_* vtable_;
    int                 value;
} noam_bool_value;

typedef struct {
    noam_value_vtable_* vtable_;
} noam_nil_value;

/* noam_op_expression struct: a binary operator
 *
 * op: string representation of operation
 * lhs, rhs: operands */
typedef struct {
    noam_expression_vtable_* vtable_;
    noam_buffer*             op;
    noam_expression*         lhs;
    noam_expression*         rhs;
} noam_op_expression;

noam_value* noam_expression_get(noam_expression* expression);
const char* noam_value_to_string(noam_value* value);
void noam_expression_release(noam_expression* expression);

int noam_value_is_instance(const noam_value* value, noam_token type);

noam_variable_expression* noam_variable_expression_create(noam_buffer* name, noam_scope* scope);
noam_value* noam_variable_expression_get(noam_variable_expression* expression);
void noam_variable_expression_release(noam_variable_expression* expression);

noam_func_call_expression* noam_func_call_expression_create(
        noam_buffer* name, noam_buffer* args, noam_symbol_table* symbol_table
);
noam_value* noam_func_call_expression_get(noam_func_call_expression* expression);
void noam_func_call_expression_release(noam_func_call_expression* expression);

noam_int_value* noam_int_value_create(int value);
const char* noam_int_value_to_string(noam_int_value* value);
noam_value* noam_int_value_get(noam_int_value* value);

noam_float_value* noam_float_value_create(float value);
const char* noam_float_value_to_string(noam_float_value* value);
noam_value* noam_float_value_get(noam_float_value* value);

noam_string_value* noam_string_value_create(noam_buffer* str);
const char* noam_string_value_to_string(noam_string_value* value);
void noam_string_value_release(noam_string_value* value);
noam_value* noam_string_value_get(noam_string_value* value);

noam_bool_value* noam_bool_value_create(int value);
const char* noam_bool_value_to_string(noam_bool_value* value);
noam_value* noam_bool_value_get(noam_bool_value* value);

noam_nil_value* noam_nil_value_create();
const char* noam_nil_value_to_string(noam_nil_value* value);
noam_value* noam_nil_value_get(noam_nil_value* value);

int noam_values_equal_type(const noam_value* lhs, const noam_value* rhs, noam_token type);

noam_op_expression* noam_op_expression_create(noam_expression* lhs, noam_buffer* op, noam_expression* rhs);
void noam_op_expression_release(noam_op_expression* expression);
noam_value* noam_op_expression_get(noam_op_expression* expression);

noam_value* noam_statements_run(noam_buffer* statements);

#endif //NOAM_EXPRESSION_H
