#pragma once

#include "dragon/ast.h"
#include "dragon/core/buf.h"
#include "dragon/core/str.h"
#include "dragon/core/sum.h"
#include "dragon/lexer.h"
#include "dragon/token.h"

typedef BUF(Token) TokenBuf;

typedef struct {
	Lexer lexer;
	TokenBuf buffer;
} Parser;

Parser parser_new(str source, str filename);

typedef RESULT(Program, str) ProgramResult;

ProgramResult parser_parse(Parser* parser);
void parser_free(Parser parser);
