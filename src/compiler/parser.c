#include "dragon/parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "dragon/core/macro.h"
#include "dragon/core/sum.h"

static MaybeToken peek(Parser* parser, uint64_t n)
{
	while (n >= parser->buffer.len) {
		if (lexer_done(&parser->lexer)) {
			return (MaybeToken)NOTHING;
		}
		Token token = lexer_next(&parser->lexer);
		BUF_PUSH(&parser->buffer, token);
	}

	return (MaybeToken)JUST(parser->buffer.ptr[n]);
}

static MaybeToken advance(Parser* parser)
{
	MaybeToken token = peek(parser, 0);
	if (token.present) {
		BUF_POP_FIRST(&parser->buffer);
	}
	return token;
}

static bool advance_ignore(Parser* parser)
{
	MaybeToken token = advance(parser);
	if (token.present) {
		token_free(token.value);
	}
	return token.present;
}

typedef RESULT(Token, str) TokenResult;

static TokenResult advance_nonnull(Parser* parser, str expected)
{
	MaybeToken token = advance(parser);
	if (!token.present) {
		if (str_len(expected) == 0) {
			return (TokenResult)ERR(str_lit("Unexpected end of file"));
		}
		str msg =
		        str_fmt(
		                "Unexpected end of file (expected " STR_FMT ")",
		                STR_ARG(expected)
		        );
		return (TokenResult)ERR(msg);
	}
	return (TokenResult)OK(token.value);
}

static TokenResult expect(Parser* parser, TokenType type)
{
	Token result;
	if (parser->buffer.len > 0) {
		result = parser->buffer.ptr[0];
		BUF_POP_FIRST(&parser->buffer);
	} else {
		TokenResult temp = advance_nonnull(parser, str_ref(TOKEN_STRINGS[type]));
		if (!temp.ok) {
			return temp;
		}
		result = temp.get.value;
	}
	if (result.type != type) {
		str msg =
		        str_fmt(
		                "Unexpected token %s (expected %s)",
		                TOKEN_STRINGS[result.type],
		                TOKEN_STRINGS[type]
		        );
		return (TokenResult)ERR(msg);
	}
	return (TokenResult)OK(result);
}

typedef MAYBE(str) ExpectErr;

static ExpectErr expect_ignore(Parser* parser, TokenType type)
{
	TokenResult result = expect(parser, type);
	if (!result.ok) {
		return (ExpectErr)JUST(result.get.error);
	}
	token_free(result.get.value);
	return (ExpectErr)NOTHING;
}

static bool look(Parser* parser, TokenType type)
{
	MaybeToken token = peek(parser, 0);
	return token.present && token.value.type == type;
}

typedef BUF(const TokenType) TokenTypeBuf;

static MaybeToken match(Parser* parser, TokenTypeBuf types)
{
	for (uint64_t i = 0; i < types.len; i++) {
		if (look(parser, types.ptr[i])) {
			return advance(parser);
		}
	}
	return (MaybeToken)NOTHING;
}

Parser parser_new(str source, str filename)
{
	Parser parser = {
		.lexer = lexer_new(source, filename),
		.buffer = BUF_NEW,
	};
	BUF_PUSH(&parser.buffer, lexer_first(&parser.lexer));
	return parser;
}

typedef RESULT(Expression*, str) ExpressionResult;

static UnaryOpKind unary_op_kind_from_token_type(TokenType type)
{
	switch (type) {
	case TT_MINUS:
		return UNARY_OP_KIND_ARITHMETIC_NEGATION;
	case TT_BANG:
		return UNARY_OP_KIND_LOGICAL_NEGATION;
	case TT_TILDE:
		return UNARY_OP_KIND_BITWISE_NEGATION;
	default:
		UNREACHABLE();
	}
}

static ExpressionResult parse_expression(Parser* parser);

static ExpressionResult parse_primary_expression(Parser* parser)
{
	if (look(parser, TT_LPAREN)) {
		advance_ignore(parser);
		ExpressionResult result = parse_expression(parser);
		if (!result.ok) {
			return result;
		}
		ExpectErr err = expect_ignore(parser, TT_RPAREN);
		if (err.present) {
			expression_free(result.get.value);
			return (ExpressionResult)ERR(err.value);
		}
		return result;
	}
	TokenResult result = expect(parser, TT_NUM);
	if (!result.ok) {
		return (ExpressionResult)ERR(result.get.error);
	}
	Token tok = result.get.value;
	int64_t num = tok.value.get.num;
	token_free(tok);
	ConstantExpression* constant = malloc(sizeof(ConstantExpression));
	constant->base.type = EXPRESSION_TYPE_CONSTANT;
	constant->number = num;
	return (ExpressionResult)OK(&constant->base);
}

static ExpressionResult parse_unary_expression(Parser* parser)
{
	const TokenType unaryOps[] = {
		TT_MINUS,
		TT_TILDE,
		TT_BANG,
	};
	MaybeToken op = match(parser, (TokenTypeBuf)BUF_ARRAY(unaryOps));
	if (op.present) {
		ExpressionResult right = parse_unary_expression(parser);
		if (!right.ok) {
			return right;
		}
		UnaryOpExpression* unary = malloc(sizeof(UnaryOpExpression));
		unary->base.type = EXPRESSION_TYPE_UNARY_OP;
		unary->kind = unary_op_kind_from_token_type(op.value.type);
		unary->operand = right.get.value;
		token_free(op.value);
		return (ExpressionResult)OK(&unary->base);
	}

	return parse_primary_expression(parser);
}

static ExpressionResult parse_multiplicative_expression(Parser* parser)
{
	ExpressionResult left = parse_unary_expression(parser);
	if (!left.ok) {
		return left;
	}

	Expression* result = left.get.value;
	MaybeToken op;

	TokenType multiplicativeTypes[] = {
		TT_STAR,
		TT_SLASH,
	};

	while ((op = match(parser, (TokenTypeBuf)BUF_ARRAY(multiplicativeTypes))).present) {
		ExpressionResult right = parse_unary_expression(parser);
		if (!right.ok) {
			expression_free(result);
			return right;
		}
		BinaryOpExpression* binary = malloc(sizeof(BinaryOpExpression));
		binary->base.type = EXPRESSION_TYPE_BINARY_OP;
		binary->kind = op.value.type == TT_STAR
		               ? BINARY_OP_KIND_MULTIPLICATION
		               : BINARY_OP_KIND_DIVISION;
		binary->left = result;
		binary->right = right.get.value;
		result = &binary->base;
		token_free(op.value);
	}

	return (ExpressionResult)OK(result);
}

static ExpressionResult parse_additive_expression(Parser* parser)
{
	ExpressionResult left = parse_multiplicative_expression(parser);
	if (!left.ok) {
		return left;
	}

	Expression* result = left.get.value;
	MaybeToken op;

	TokenType additiveTypes[] = {
		TT_PLUS,
		TT_MINUS,
	};

	while ((op = match(parser, (TokenTypeBuf)BUF_ARRAY(additiveTypes))).present) {
		ExpressionResult right = parse_multiplicative_expression(parser);
		if (!right.ok) {
			expression_free(result);
			return right;
		}
		BinaryOpExpression* binary = malloc(sizeof(BinaryOpExpression));
		binary->base.type = EXPRESSION_TYPE_BINARY_OP;
		binary->kind = op.value.type == TT_PLUS
		               ? BINARY_OP_KIND_ADDITION
		               : BINARY_OP_KIND_SUBTRACTION;
		binary->left = result;
		binary->right = right.get.value;
		result = &binary->base;
		token_free(op.value);
	}

	return (ExpressionResult)OK(result);
}

static ExpressionResult parse_relational_expression(Parser* parser)
{
	ExpressionResult left = parse_additive_expression(parser);
	if (!left.ok) {
		return left;
	}

	Expression* result = left.get.value;
	MaybeToken op;

	TokenType relationalTypes[] = {
		TT_LEFT,
		TT_RIGHT,
		TT_LEFT_EQUAL,
		TT_RIGHT_EQUAL,
	};

	while ((op = match(parser, (TokenTypeBuf)BUF_ARRAY(relationalTypes))).present) {
		ExpressionResult right = parse_additive_expression(parser);
		if (!right.ok) {
			expression_free(result);
			return right;
		}
		BinaryOpExpression* binary = malloc(sizeof(BinaryOpExpression));
		binary->base.type = EXPRESSION_TYPE_BINARY_OP;
		switch (op.value.type) {
		case TT_LEFT:
			binary->kind = BINARY_OP_KIND_LESS;
			break;
		case TT_RIGHT:
			binary->kind = BINARY_OP_KIND_GREATER;
			break;
		case TT_LEFT_EQUAL:
			binary->kind = BINARY_OP_KIND_LESS_EQUAL;
			break;
		case TT_RIGHT_EQUAL:
			binary->kind = BINARY_OP_KIND_GREATER_EQUAL;
			break;
		default:
			UNREACHABLE();
		}
		binary->left = result;
		binary->right = right.get.value;
		result = &binary->base;
		token_free(op.value);
	}

	return (ExpressionResult)OK(result);
}

static ExpressionResult parse_equality_expression(Parser* parser)
{
	ExpressionResult left = parse_relational_expression(parser);
	if (!left.ok) {
		return left;
	}

	Expression* result = left.get.value;
	MaybeToken op;

	TokenType equalityTypes[] = {
		TT_EQUAL_EQUAL,
		TT_BANG_EQUAL,
	};

	while ((op = match(parser, (TokenTypeBuf)BUF_ARRAY(equalityTypes))).present) {
		ExpressionResult right = parse_relational_expression(parser);
		if (!right.ok) {
			expression_free(result);
			return right;
		}
		BinaryOpExpression* binary = malloc(sizeof(BinaryOpExpression));
		binary->base.type = EXPRESSION_TYPE_BINARY_OP;
		binary->kind = op.value.type == TT_EQUAL_EQUAL
		               ? BINARY_OP_KIND_EQUALITY
		               : BINARY_OP_KIND_INEQUALITY;
		binary->left = result;
		binary->right = right.get.value;
		result = &binary->base;
		token_free(op.value);
	}

	return (ExpressionResult)OK(result);
}

static ExpressionResult parse_logical_and_expression(Parser* parser)
{
	ExpressionResult left = parse_equality_expression(parser);
	if (!left.ok) {
		return left;
	}

	Expression* result = left.get.value;

	while (look(parser, TT_AMP_AMP)) {
		advance_ignore(parser);
		ExpressionResult right = parse_equality_expression(parser);
		if (!right.ok) {
			expression_free(result);
			return right;
		}
		BinaryOpExpression* binary = malloc(sizeof(BinaryOpExpression));
		binary->base.type = EXPRESSION_TYPE_BINARY_OP;
		binary->kind = BINARY_OP_KIND_LOGICAL_AND;
		binary->left = result;
		binary->right = right.get.value;
		result = &binary->base;
	}

	return (ExpressionResult)OK(result);
}

static ExpressionResult parse_logical_or_expression(Parser* parser)
{
	ExpressionResult left = parse_logical_and_expression(parser);
	if (!left.ok) {
		return left;
	}

	Expression* result = left.get.value;

	while (look(parser, TT_PIPE_PIPE)) {
		advance_ignore(parser);
		ExpressionResult right = parse_logical_and_expression(parser);
		if (!right.ok) {
			expression_free(result);
			return right;
		}
		BinaryOpExpression* binary = malloc(sizeof(BinaryOpExpression));
		binary->base.type = EXPRESSION_TYPE_BINARY_OP;
		binary->kind = BINARY_OP_KIND_LOGICAL_OR;
		binary->left = result;
		binary->right = right.get.value;
		result = &binary->base;
	}

	return (ExpressionResult)OK(result);
}

static ExpressionResult parse_expression(Parser* parser)
{
	return parse_logical_or_expression(parser);
}

typedef RESULT(Statement, str) StatementResult;

static StatementResult parse_statement(Parser* parser)
{
	ExpectErr err = expect_ignore(parser, KW_RETURN);
	if (err.present) {
		return (StatementResult)ERR(err.value);
	}
	ExpressionResult expr = parse_expression(parser);
	if (!expr.ok) {
		return (StatementResult)ERR(expr.get.error);
	}
	err = expect_ignore(parser, TT_SEMI);
	if (err.present) {
		expression_free(expr.get.value);
		return (StatementResult)ERR(err.value);
	}
	return (StatementResult)OK((Statement) { .expression = expr.get.value });
}

typedef RESULT(Function, str) FunctionResult;

static FunctionResult parse_function(Parser* parser)
{
	ExpectErr err = expect_ignore(parser, KW_INT);
	if (err.present) {
		return (FunctionResult)ERR(err.value);
	}
	TokenResult id = expect(parser, TT_IDENT);
	if (!id.ok) {
		return (FunctionResult)ERR(id.get.error);
	}
	err = expect_ignore(parser, TT_LPAREN);
	if (err.present) {
		token_free(id.get.value);
		return (FunctionResult)ERR(err.value);
	}
	err = expect_ignore(parser, TT_RPAREN);
	if (err.present) {
		token_free(id.get.value);
		return (FunctionResult)ERR(err.value);
	}
	err = expect_ignore(parser, TT_LBRACE);
	if (err.present) {
		token_free(id.get.value);
		return (FunctionResult)ERR(err.value);
	}
	StatementResult stmt = parse_statement(parser);
	if (!stmt.ok) {
		token_free(id.get.value);
		return (FunctionResult)ERR(stmt.get.error);
	}
	err = expect_ignore(parser, TT_RBRACE);
	if (err.present) {
		expression_free(stmt.get.value.expression);
		token_free(id.get.value);
		return (FunctionResult)ERR(err.value);
	}
	str name = str_move(&id.get.value.value.get.str);
	token_free(id.get.value);
	return (FunctionResult)OK(((Function) {
		.name = name,
		.statement = stmt.get.value,
	}));
}

ProgramResult parser_parse(Parser* parser)
{
	FunctionResult result = parse_function(parser);
	if (!result.ok) {
		return (ProgramResult)ERR(result.get.error);
	}
	return (ProgramResult)OK(((Program) {
		.function = result.get.value,
	}));
}

void parser_free(Parser parser)
{
	for (uint64_t i = 0; i < parser.buffer.len; i++) {
		token_free(parser.buffer.ptr[i]);
	}
	BUF_FREE(parser.buffer);
}
