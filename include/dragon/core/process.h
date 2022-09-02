#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#include "dragon/core/buf.h"
#include "dragon/core/sum.h"

typedef struct {
	FILE* stdinFile;
	FILE* stdoutFile;
	FILE* stderrFile;

	pid_t child;
	int returnCode;

	bool alive;
} Process;

typedef enum {
#define X(x) PROCESS_OPTION_##x,
#include "process_options.def"

#undef X
} ProcessOption;

typedef BUF(const char*) ProcessCStrBuf;

typedef MAYBE(Process) ProcessCreateResult;

ProcessCreateResult process_create(ProcessCStrBuf commandLine, ProcessOption options);

typedef MAYBE(int) ProcessJoinResult;

ProcessJoinResult process_join(Process* process);

void process_destroy(Process* process);

ProcessCreateResult process_run(ProcessCStrBuf commandLine, ProcessOption options);
