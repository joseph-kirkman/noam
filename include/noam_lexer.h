#ifndef NOAM_LEXER_H
#define NOAM_LEXER_H

#include "noam_utility.h"
#include "noam_buffer.h"

/* Tokens strings */
#define NOAM_IF_STR "if"
#define NOAM_ELSE_STR "else"
#define NOAM_PRINT_STR "print"
#define NOAM_FUNC_STR "func"
#define NOAM_TRUE_STR "true"
#define NOAM_FALSE_STR "false"
#define NOAM_RETURN_STR "return"
#define NOAM_EQ_STR "="
#define NOAM_PLUS_STR "+"
#define NOAM_MINUS_STR "-"
#define NOAM_MULT_STR "*"
#define NOAM_DIV_STR "/"
#define NOAM_LESS_STR "<"
#define NOAM_GREATER_STR ">"
#define NOAM_LP_STR "("
#define NOAM_RP_STR ")"
#define NOAM_LB_STR "{"
#define NOAM_RB_STR "}"
#define NOAM_EQ2_STR "=="
#define NOAM_NEQ_STR "!="
#define NOAM_COMMA_STR ","
#define NOAM_NIL_STR "nil"

/* noam_state enum: tokens parsing state */
typedef enum {
    NOAM_DEFAULT_STATE,
    NOAM_WORD_STATE,
    NOAM_STRING_STATE,
    NOAM_NUMBER_STATE,
    NOAM_FRACTION_STATE,
    NOAM_COMMENT_STATE,
    NOAM_SPEC_STATE
} noam_state;

/* noam_token enum */
typedef enum {
    NOAM_ERROR_TOKEN,
    NOAM_WORD_TOKEN,
    NOAM_INT_TOKEN,
    NOAM_FLOAT_TOKEN,
    NOAM_STRING_TOKEN,
    NOAM_BOOL_TOKEN,
    NOAM_LINE_TOKEN,
    NOAM_EQ_TOKEN,
    NOAM_OP_TOKEN,
    NOAM_LP_TOKEN,
    NOAM_RP_TOKEN,
    NOAM_LB_TOKEN,
    NOAM_RB_TOKEN,
    NOAM_COMMA_TOKEN,
    NOAM_NIL_TOKEN,
    NOAM_EOF_TOKEN
} noam_token;

/* noam_prefix_node struct: a prefix tree node for parsing keywords
 *
 * parent, next, child: pointers to a specific nodes in a prefix tree
 * character: char in a string represented as a tree
 * token: NOAM_ERROR_TOKEN if it's an internal node, set to a specific value if node is a leaf
 * */
typedef struct noam_prefix_node {
    struct noam_prefix_node* parent;
    struct noam_prefix_node* next;
    struct noam_prefix_node* child;
    char                     character;
    noam_token               token;
} noam_prefix_node;

/* noam_token_info struct: describes information corresponding to token
 *
 * name: string representation
 * token: token type */
typedef struct {
    noam_buffer* name;
    noam_token   token;
} noam_token_info;

noam_token_info* noam_token_info_create(noam_buffer* name, noam_token token);
void noam_token_info_release(noam_token_info* info);


noam_prefix_node* noam_prefix_node_create(char character);
noam_prefix_node* noam_prefix_node_add_child(noam_prefix_node* parent, char character);
noam_prefix_node* noam_prefix_node_add_sibling(noam_prefix_node* prev, char character);

void noam_prefix_tree_insert(noam_prefix_node* root, const char* keyword, noam_token token);
int noam_prefix_tree_contains(noam_prefix_node* root, noam_buffer* keyword);
noam_token noam_prefix_tree_find(noam_prefix_node* root, noam_buffer* keyword);
noam_prefix_node* noam_prefix_tree_build(const char** keywords, const noam_token* tokens, size_t length);
noam_prefix_node* noam_tokens_tree();

void noam_state_reset(noam_buffer* token_name, noam_state* state);
noam_buffer* noam_parse_tokens(const char* source);

#endif //NOAM_LEXER_H
