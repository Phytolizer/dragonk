#pragma once

#include <stdint.h>

#include "dragon/core/str.h"

typedef enum {
#define X(x) EXPRESSION_TYPE_##x,
#include "dragon/expr_types.def"
#undef X
} ExpressionType;

typedef struct {
	ExpressionType type;
} Expression;

typedef enum {
#define X(x) UNARY_OP_KIND_##x,
#include "dragon/unary_op_kinds.def"
#undef X
} UnaryOpKind;

typedef struct {
	Expression base;
	UnaryOpKind kind;
	Expression* operand;
} UnaryOpExpression;

typedef struct {
	Expression base;
	int64_t number;
} ConstantExpression;

typedef struct {
	Expression* expression;
} Statement;

typedef struct {
	str name;
	Statement statement;
} Function;

typedef struct {
	Function function;
} Program;

str program_to_str(Program program);
void program_free(Program program);
