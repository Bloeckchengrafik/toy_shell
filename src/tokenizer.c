#include "tokenizer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static token_t *create_token(const char *string, token_type_t type) {
  token_t *token = (token_t *)malloc(sizeof(token_t));
  if (!token) {
    perror("malloc");
    exit(1);
  }
  token->string = strdup(string);
  if (!token->string) {
    perror("strdup");
    exit(1);
  }
  token->type = type;
  token->next = NULL;
  return token;
}

static void append_token(token_t **head, token_t *new_token) {
  if (!*head) {
    *head = new_token;
  } else {
    token_t *current = *head;
    while (current->next) {
      current = current->next;
    }
    current->next = new_token;
  }
}

static const char *parse_spaces(const char *input, char **output) {
  const char *start = input;
  while (*input && isspace((unsigned char)*input)) {
    input++;
  }
  size_t length = input - start;
  *output = (char *)malloc(length + 1);
  if (!*output) {
    perror("malloc");
    exit(1);
  }
  strncpy(*output, start, length);
  (*output)[length] = '\0';
  return input;
}

static const char *parse_quoted_string(const char *input, char **output) {
  const char *start = input;
  input++;

  while (*input && *input != '"') {
    input++;
  }

  size_t length = (*input == '"') ? (input - start + 1) : (input - start);

  *output = (char *)malloc(length + 1);
  if (!*output) {
    perror("malloc");
    exit(1);
  }

  strncpy(*output, start, length);
  (*output)[length] = '\0';

  if (*input == '"') {
    input++;
  }

  return input;
}

static const char *parse_token(const char *input, char **output) {
  const char *start = input;
  while (*input && !isspace((unsigned char)*input) && *input != '&') {
    input++;
  }
  size_t length = input - start;
  *output = (char *)malloc(length + 1);
  if (!*output) {
    perror("malloc");
    exit(1);
  }
  strncpy(*output, start, length);
  (*output)[length] = '\0';
  return input;
}

token_t *tokenize(const char *input) {
  token_t *tokens = NULL;
  bool is_first_token = true;

  while (*input) {
    char *token_string = NULL;
    token_type_t token_type;

    if (isspace((unsigned char)*input)) {
      input = parse_spaces(input, &token_string);
      token_type = TOKEN_SPACE;
    } else if (*input == '"') {
      input = parse_quoted_string(input, &token_string);
      token_type = is_first_token ? TOKEN_COMMAND : TOKEN_QUOTED_STR;
      is_first_token = false;
    } else if (*input == '&') {
      token_string = strdup("&");
      if (!token_string) {
        perror("strdup");
        exit(1);
      }
      token_type = TOKEN_CONTINUE;
      append_token(&tokens, create_token(token_string, token_type));
      free(token_string);
      break;
    } else {
      input = parse_token(input, &token_string);
      if (is_first_token) {
        token_type = TOKEN_COMMAND;
        is_first_token = false;
      } else if (token_string[0] == '-') {
        token_type = TOKEN_PARAMETER;
      } else {
        token_type = TOKEN_ARGUMENT;
      }
    }

    if (token_string) {
      append_token(&tokens, create_token(token_string, token_type));
      free(token_string);
    }
  }

  return tokens;
}

void free_tokens(token_t *tokens) {
  while (tokens) {
    token_t *next = tokens->next;
    free(tokens->string);
    free(tokens);
    tokens = next;
  }
}

char **tokens_to_command(token_t *token) {
  size_t count = 0;
  token_t *current = token;

  while (current) {
    if (current->type != TOKEN_SPACE || current->type != TOKEN_CONTINUE) {
      count++;
    }
    current = current->next;
  }

  char **command = (char **)malloc((count + 1) * sizeof(char *));
  if (!command) {
    perror("malloc");
    exit(1);
  }

  current = token;
  size_t index = 0;

  while (current) {
    if (current->type != TOKEN_SPACE && current->type != TOKEN_CONTINUE) {
      command[index] = strdup(current->string);
      if (!command[index]) {
        perror("strdup");
        exit(1);
      }

      if (command[index][0] == '"' &&
          command[index][strlen(command[index]) - 1] == '"') {
        memmove(command[index], command[index] + 1, strlen(command[index]) - 1);
        command[index][strlen(command[index]) - 2] = '\0';
      }

      index++;
    }
    current = current->next;
  }

  command[index] = NULL;
  return command;
}

void free_command(char **command) {
  if (!command) {
    return;
  }

  for (size_t i = 0; command[i] != NULL; i++) {
    free(command[i]);
  }

  free(command);
}

bool should_continue(token_t *token) {
  while (token) {
    if (token->type == TOKEN_CONTINUE) {
      return true;
    }
    token = token->next;
  }
  return false;
}
