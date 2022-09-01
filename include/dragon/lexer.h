#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dragon/core/str.h"
#include "dragon/core/sum.h"
#include "dragon/token.h"

typedef struct {
	str source;
	SourceLocation tokenStartLoc;
	uint64_t tokenStart;
	uint64_t pos;
	str filename;
	uint64_t line;
	uint64_t column;
	Token lookahead;
	bool canLexHeaderName;
} Lexer;

Lexer lexer_new(str source, str filename);

typedef MAYBE(Token) MaybeToken;

Token lexer_first(Lexer* lexer);
bool lexer_done(Lexer* lexer);
Token lexer_next(Lexer* lexer);
