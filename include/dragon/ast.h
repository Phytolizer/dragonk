#pragma once

#include "dragon/core/str.h"

typedef struct {
	int64_t number;
} Expression;

typedef struct {
	Expression expression;
} Statement;

typedef struct {
	str name;
	Statement statement;
} Function;

typedef struct {
	str name;
} Header;

typedef struct {
	Header header;
	Function function;
} Program;

str program_to_str(Program program);
void program_free(Program program);
