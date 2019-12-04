#include "noam_symbol.h"
#include "noam_expression.h"

void noam_scope_vars_release(void* data){
    noam_buffer_release(data);
    noam_expression_release(*(noam_expression**)((char*)data + sizeof(noam_buffer)));
}

noam_scope* noam_scope_create(noam_buffer* name){
    noam_scope* scope = malloc(sizeof(noam_scope));
    memset(scope, 0, sizeof(noam_scope));
    scope->vars = noam_dict_createv(sizeof(noam_buffer), sizeof(noam_value*),
                                    &noam_hash_string, &noam_cmp_string,
                                    &noam_scope_vars_release);
    scope->name = name;
    return scope;
}

noam_scope* noam_scope_add_child(noam_buffer* name, noam_scope* parent){
    noam_scope* scope = noam_scope_create(name);
    parent->child = scope;
    scope->parent = parent;
    scope->next = NULL;
    scope->child = NULL;
    return scope;
}

noam_scope* noam_scope_add_sibling(noam_buffer* name, noam_scope* prev){
    noam_scope* scope = noam_scope_create(name);
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
#ifdef NOAM_DEBUG
    printf("noam_func_create: %s\n", (char*)name->data);
#endif
    noam_func* func = malloc(sizeof(noam_func));
    func->name = name;
    func->params = params;
    func->body = body;
    return func;
}

void noam_func_release(noam_func* func){
    //TODO: Function release throws right now
    //noam_buffer_release(func->name);
    //noam_buffer_release(func->params);
    //noam_buffer_release(func->body);
}

void noam_symbol_table_funcs_release(void* data){
    noam_buffer_release(data);
    noam_func_release((noam_func*)((char*)data + sizeof(noam_buffer)));
}

noam_symbol_table* noam_symbol_table_create(){
    noam_symbol_table* symbol_table = malloc(sizeof(noam_symbol_table));
    symbol_table->head = noam_scope_create(NULL);
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
