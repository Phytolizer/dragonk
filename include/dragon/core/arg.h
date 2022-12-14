#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "dragon/core/buf.h"
#include "dragon/core/str.h"
#include "dragon/core/sum.h"

typedef enum {
	ARGKIND_POS,
	ARGKIND_OPT,
	ARGKIND_FLAG,
} ArgKind;

typedef struct {
	ArgKind kind;
	char shortname;
	str longname;
	str help;
	// set if kind == ARGKIND_OPT or ARGKIND_POS
	str value;
	// set if kind == ARGKIND_FLAG
	bool flagValue;
} Arg;

#define ARG_POS(name, hlp) \
	(Arg) { .kind = ARGKIND_POS, .longname = name, .help = hlp }

#define ARG_OPT(...) \
	(Arg) { .kind = ARGKIND_OPT, __VA_ARGS__ }

#define ARG_FLAG(...) \
	(Arg) { .kind = ARGKIND_FLAG, __VA_ARGS__ }

typedef BUF(Arg*) ArgBuf;

typedef struct {
	ArgBuf args;
	str name;
	str help;
	StrBuf extra;
} ArgParser;

typedef MAYBE(str) ArgParseErr;

ArgParser argparser_new(str name, str help, ArgBuf args);
ArgParseErr argparser_parse(ArgParser* parser, int argc, char** argv);
void argparser_show_help(ArgParser* parser, FILE* fp);
