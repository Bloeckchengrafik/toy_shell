#include "shell.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDCHARS 512

pid_t fg_proc = 0;
pid_t old_fg_proc = 0;
char *cmdbuf;

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

int numPlaces(int n) {
  if (n == 0)
    return 1;
  return floor(log10(abs(n))) + 1;
}

void handle_SIGINT(int _signal) {
    if (fg_proc == 0) return;
    kill(fg_proc, SIGINT);
    printf("\n\033[90m[%d] killed\033[97m\n", fg_proc);
}

void handle_SIGTSTP(int _signal) {
    if (fg_proc == 0) return;
    kill(fg_proc, SIGTSTP);
    old_fg_proc = fg_proc;
    printf("\n\033[90m[%d] stopped\033[97m\n", fg_proc);
}

void handle_SIGCHLD(int _signal) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
  cmdbuf = calloc(CMDCHARS, sizeof(char));
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // get winsize
  signal(SIGINT, handle_SIGINT);
  signal(SIGTSTP, handle_SIGTSTP);
  signal(SIGCHLD, handle_SIGCHLD);

  bool last_was_exit = false;
  while (true) {
    readCmd(cmdbuf);
    if (*cmdbuf == 0) {
      printf("\r\n");
      continue;
    }
    token_t *tokens = tokenize(cmdbuf);
    char **command = tokens_to_command(tokens);
    bool is_continue = should_continue(tokens);

    if (strcmp(*command, "exit") == 0) {
      if (!last_was_exit) {
        printf("\n\033[90m[To confirm, type \033[97mexit\033[90m "
               "again]\033[97m\n");
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
      char *arg = *(command + 1);
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

    if (strcmp(*command, "stop") == 0) {
      char *arg = *(command + 1);
      printf("\n");
      if (arg == NULL) {
        printf("\033[90mUsage: \033[97mstop [pid]\033[97m\n");
      } else {
        int val = atoi(arg);
        if (val == 0) {
          printf("\033[31mError\033[97m: No such file or directory\033[97m\n");
        } else {
          kill((pid_t)val, SIGTSTP);
        }
      }

      free_command(command);
      free_tokens(tokens);
      continue;
    }

    if (strcmp(*command, "cont") == 0) {
      char *arg = *(command + 1);
      printf("\n");
      if (arg == NULL) {
        printf("\033[90mUsage: \033[97mstop [pid]\033[97m\n");
      } else {
        int val = atoi(arg);
        if (val == 0) {
          printf("\033[31mError\033[97m: No such file or directory\033[97m\n");
        } else {
          kill((pid_t)val, SIGCONT);
          if (val == old_fg_proc) {
              old_fg_proc = 0;
              fg_proc = val;
              waitpid(val, NULL, 0);
          }
        }
      }

      free_command(command);
      free_tokens(tokens);
      continue;
    }

    if (strcmp(*command, "pwd") == 0) {
      char cwd[PATH_MAX];
      if (getcwd(cwd, sizeof(cwd)) == NULL) {
        cwd[0] = '?';
        cwd[1] = 0;
      }
      printf("\n%s\n\n", cwd);

      free_command(command);
      free_tokens(tokens);
      continue;
    }

    last_was_exit = false;

    pid_t pid = fork();
    if (pid == 0) {
      pid_t my_id = getpid();
      setpgid(0, my_id);
      execvp(command[0], command);
      perror("\033[31mError\033[97m");
      exit(1);
    } else if (pid > 0) {
      printf("\r\033[%dC\033[37mPID: \033[33m%d\033[97m\n",
             w.ws_col - numPlaces(pid) - 6, pid);
      if (!is_continue) {
        fg_proc = pid;
        int status;
        waitpid(pid, &status, WUNTRACED);
        fg_proc = 0;
      }
    } else {
      perror("fork");
    }
    free_command(command);
    free_tokens(tokens);
  }
}
