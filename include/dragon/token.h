#pragma once

#include "dragon/core/str.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
#define X(x) x,
#include "dragon/token_type.def"
#undef X
} TokenType;

extern const char* TOKEN_STRINGS[];

typedef struct {
	str filename;
	uint64_t line;
	uint64_t column;
} SourceLocation;

typedef enum {
	TK_NONE,
	TK_STR,
	TK_NUM,
} TokenValueKind;

typedef struct {
	TokenValueKind kind;
	union {
		str str;
		int64_t num;
	} get;
} TokenValue;

#define TOKEN_VALUE_NONE ((TokenValue) { .kind = TK_NONE })
#define TOKEN_VALUE_STR(s) ((TokenValue) { .kind = TK_STR, .get.str = s })
#define TOKEN_VALUE_NUM(n) ((TokenValue) { .kind = TK_NUM, .get.num = n })

typedef struct {
	TokenType type;
	SourceLocation location;
	str text;
	TokenValue value;
} Token;

void token_free(Token tok);

#define SOURCE_LOCATION_FMT "%s:%" PRIu64 ":%" PRIu64
#define SOURCE_LOCATION_ARG(loc) (loc).filename.ptr, (loc).line, (loc).column

void token_value_show(TokenValue val, FILE* fp);
