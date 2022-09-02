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

static ExpressionResult parse_expression(Parser* parser)
{
	const TokenType unaryOps[] = {
		TT_MINUS,
		TT_TILDE,
		TT_BANG,
	};
	MaybeToken op = match(parser, (TokenTypeBuf)BUF_ARRAY(unaryOps));
	if (op.present) {
		ExpressionResult right = parse_expression(parser);
		if (!right.ok) {
			return right;
		}
		UnaryOpExpression* unary = malloc(sizeof(UnaryOpExpression));
		unary->base.type = EXPRESSION_TYPE_UNARY_OP;
		unary->kind = unary_op_kind_from_token_type(op.value.type);
		token_free(op.value);
		unary->operand = right.get.value;
		return (ExpressionResult)OK(&unary->base);
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
		return (FunctionResult)ERR(err.value);
	}
	err = expect_ignore(parser, TT_RPAREN);
	if (err.present) {
		return (FunctionResult)ERR(err.value);
	}
	err = expect_ignore(parser, TT_LBRACE);
	if (err.present) {
		return (FunctionResult)ERR(err.value);
	}
	StatementResult stmt = parse_statement(parser);
	if (!stmt.ok) {
		return (FunctionResult)ERR(stmt.get.error);
	}
	err = expect_ignore(parser, TT_RBRACE);
	if (err.present) {
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
	BUF_FREE(parser.buffer);
}
