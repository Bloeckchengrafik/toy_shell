#include "shell.h"
#include "tokenizer.h"
#include <signal.h>
#include <math.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDCHARS 512

void handle_SIGINT(int _signal) {
    printf("\n\n\033[90m[To quit, type \033[97mexit\033[90m twice]\033[97m\n\n");
}

void outputShell(char *cmdbuf) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    cwd[0] = '?';
    cwd[1] = 0;
  }
  printf("\033[97m\r%s\033[90m $ \033[97m", cwd);
  token_t *tokens = tokenize(cmdbuf);

  for (token_t *token = tokens; token; token = token->next) {
    switch (token->type) {
    case TOKEN_COMMAND:
      printf("\033[33m%s", token->string);
      break;
    case TOKEN_SPACE:
      printf("\033[33m%s", token->string);
      break;
    case TOKEN_ARGUMENT:
      printf("\033[97m%s", token->string);
      break;
    case TOKEN_PARAMETER:
      printf("\033[90m%s", token->string);
      break;
    case TOKEN_CONTINUE:
      printf("\033[90m%s", token->string);
      break;
    case TOKEN_QUOTED_STR:
      printf("\033[34m%s", token->string);
      break;
    }
  }

  printf(" \033[1D\033[97m");

  free_tokens(tokens);
}

void readCmd(char *cmdbuf) {
  memset(cmdbuf, 0, CMDCHARS);

  int cmdIndex = 0;
  for (cmdIndex = 0; cmdIndex < CMDCHARS; cmdIndex++) {
    outputShell(cmdbuf);
    char c = getCh();
    if (c == 27) {
      getCh();
      getCh();
      cmdIndex--;
      continue;
    }
    if (c == '\n' || c == '\0') {
      cmdbuf[cmdIndex] = '\0';
      break;
    }
    if (c == '\b' || c == 127) {
      if (cmdIndex > 0) {
        cmdIndex--;
        cmdbuf[cmdIndex] = '\0';
        cmdIndex--;
      } else {
        cmdIndex--;
      }
      continue;
    }
    cmdbuf[cmdIndex] = c;
    if (cmdIndex == 511) {
      printf("%c", TERM_BELL);
    }
  }
}

int numPlaces (int n) {
    if (n == 0) return 1;
    return floor (log10 (abs (n))) + 1;
}

int main() {
  char *cmdbuf = calloc(CMDCHARS, sizeof(char));
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  signal(SIGINT, handle_SIGINT);

  bool last_was_exit = false;
  while (true) {
    readCmd(cmdbuf);
    if (*cmdbuf == 0) {
        printf("\n");
        continue;
    }
    token_t *tokens = tokenize(cmdbuf);
    char **command = tokens_to_command(tokens);
    bool is_continue = should_continue(tokens);

    if (strcmp(*command, "exit") == 0) {
        if (!last_was_exit) {
            printf("\n\033[90m[To confirm, type \033[97mexit\033[90m again]\033[97m\n");
            last_was_exit = true;
        } else {
            printf("\n\033[90m[Bye]\033[97m\n");
            exit(0);
        }
        free_command(command);
        free_tokens(tokens);
        continue;
    }

    if (strcmp(*command, "cd") == 0) {
        char *arg = *(command+1);
        printf("\n");
        if (arg == NULL) {
            printf("\033[90mUsage: \033[97mcd [path]\033[97m\n");
        } else {
            if (chdir(arg) != 0) {
                printf("\033[31mError\033[97m: No such file or directory\033[97m\n");
            }
        }

        free_command(command);
        free_tokens(tokens);
        continue;
    }

    last_was_exit = false;

    pid_t pid = fork();
    if (pid == 0) {
      execvp(command[0], command);
      perror("\033[31mError\033[97m");
      exit(1);
    } else if (pid > 0) {
      printf("\r\033[%dC\033[37mPID: \033[33m%d\033[97m\n", w.ws_col - numPlaces(pid) - 6, pid);
      if (!is_continue) {
        int status;
        waitpid(pid, &status, 0);
      }
    } else {
      perror("fork");
    }
    free_command(command);
    free_tokens(tokens);
  }
}
