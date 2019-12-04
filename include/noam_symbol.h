#ifndef NOAM_SYMBOL_H
#define NOAM_SYMBOL_H

#include "noam_buffer.h"
#include "noam_dict.h"

/* noam_scope struct: scope of the program which can be a function scope or a block scope
 *
 * vars: variables dictionary or a mapping from string to noam_expression
 * name: NULL is used for ordinary scopes, function name for functions
 * parent, next, child: neighbour nodes in order to traverse the scopes tree
 *
 * root of the tree is a main scope, all functions params are on the first level scope,
 * variables in blocks are placed down to the bottom of the tree
 * */
typedef struct noam_scope {
    noam_dict*         vars;
    noam_buffer*       name;
    struct noam_scope* parent;
    struct noam_scope* next;
    struct noam_scope* child;
} noam_scope;


/* noam_func struct: a function
 *
 * name: function name
 * params: plain params strings
 * body: array of noam_statements
 * */
typedef struct {
    noam_buffer* name;
    noam_buffer* params;
    noam_buffer* body;
} noam_func;

/* noam_symbol_table: symbol table for the program
 *
 * funcs: a mapping from function name to function struct
 * head: a root of the scopes tree
 * */
typedef struct {
    noam_dict* funcs;
    noam_scope* head;
} noam_symbol_table;

void noam_scope_vars_release(void* data);

noam_scope* noam_scope_create(noam_buffer* name);
noam_scope* noam_scope_add_child(noam_buffer* name, noam_scope* parent);
noam_scope* noam_scope_add_sibling(noam_buffer* name, noam_scope* prev);
void noam_scope_release(noam_scope* head);

noam_func* noam_func_create(noam_buffer* name, noam_buffer* params, noam_buffer* body);
void noam_func_release(noam_func* func);
void noam_symbol_table_funcs_release(void* data);

noam_symbol_table* noam_symbol_table_create();
void noam_symbol_table_release(noam_symbol_table* symbol_table);

#endif //NOAM_SYMBOL_H
