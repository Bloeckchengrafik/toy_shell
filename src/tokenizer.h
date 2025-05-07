#pragma once

#include <stdbool.h>

typedef enum {
    TOKEN_COMMAND,
    TOKEN_ARGUMENT,
    TOKEN_PARAMETER,
    TOKEN_CONTINUE,
    TOKEN_SPACE,
    TOKEN_QUOTED_STR
} token_type_t;

typedef struct token_t {
    char *string;
    token_type_t type;
    struct token_t *next;
} token_t;

token_t *tokenize(const char *input);
void free_tokens(token_t *tokens);
char **tokens_to_command(token_t *token);
void free_command(char **command);
bool should_continue(token_t *token);
