#include <unistd.h>
#include <termios.h>
#include <stdio.h>

#include "shell.h"

void noEcho(bool enable) {
    if (enable) {
        struct termios settings = {};
        tcgetattr(STDIN_FILENO, &settings);
        settings.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &settings);
    } else {
        struct termios settings = {};
        tcgetattr(STDIN_FILENO, &settings);
        settings.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &settings);
    }
}

char getCh() {
    struct termios oldt = {}, newt = {};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    const char ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
