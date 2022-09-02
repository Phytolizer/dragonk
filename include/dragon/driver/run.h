#pragma once

#include "dragon/core/buf.h"
#include <stdio.h>

typedef BUF(char*) CArgBuf;

int run(CArgBuf args, FILE* out, FILE* err);
