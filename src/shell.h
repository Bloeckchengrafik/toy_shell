#pragma once
#include <stdbool.h>

#define CH_ENTER     10
#define CH_BACKSPACE 127

#define TERM_BELL    '\07'

void noEcho(bool enable);
char getCh();
