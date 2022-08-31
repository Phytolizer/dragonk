#include "dragon/lexer.h"

#include "dragon/core/macro.h"
#include "dragon/core/sum.h"
#include "dragon/gperf/keywords.h"
#include "dragon/gperf/ppkeywords.h"
#include <stdio.h>
#include <stdlib.h>

typedef MAYBE(char) MaybeChar;

static MaybeChar lexer_peek(Lexer* lexer, uint64_t distance)
{
	if (lexer->pos + distance >= str_len(lexer->source)) {
		return (MaybeChar)NOTHING;
	}

	return (MaybeChar)JUST(lexer->source.ptr[lexer->pos + distance]);
}

static MaybeChar lexer_advance(Lexer* lexer)
{
	MaybeChar c = lexer_peek(lexer, 0);
	if (c.present) {
		lexer->pos++;

		if (c.value == '\n') {
			lexer->line++;
			lexer->column = 1;
		} else {
			lexer->column++;
		}
	}

	return c;
}

static str lexer_text(Lexer* lexer)
{
	const char* text_begin = &lexer->source.ptr[lexer->tokenStart];
	uint64_t text_len = lexer->pos - lexer->tokenStart;
	return str_ref_chars(text_begin, text_len);
}

static Token make_token(Lexer* lexer, TokenType type, TokenValue value)
{
	return (Token) {
		.type = type,
		.location = lexer->tokenStartLoc,
		.text = str_copy(lexer_text(lexer)),
		.value = value,
	};
}

static bool char_is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool char_is_letter(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool char_is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static bool char_is_letter_or_digit(char c)
{
	return char_is_letter(c) || char_is_digit(c);
}

static void skip_whitespace(Lexer* lexer)
{
	MaybeChar c = lexer_peek(lexer, 0);
	while (c.present) {
		if (char_is_space(c.value)) {
			lexer_advance(lexer);
		} else if (c.value == '/') {
			// possibly a comment?
			MaybeChar next = lexer_peek(lexer, 1);
			if (next.present && next.value == '/') {
				// found a `//` comment
				lexer_advance(lexer);
				lexer_advance(lexer);
				while (true) {
					c = lexer_peek(lexer, 0);
					if (!c.present || c.value == '\n') {
						break;
					}
					lexer_advance(lexer);
				}
			} else {
				// nevermind
				break;
			}
		} else {
			break;
		}
		c = lexer_peek(lexer, 0);
	}
}

static Token lex_ident_or_kw(Lexer* lexer)
{
	while (true) {
		MaybeChar c = lexer_peek(lexer, 0);
		if (!c.present || !char_is_letter_or_digit(c.value)) {
			break;
		}
		lexer_advance(lexer);
	}

	str text = lexer_text(lexer);
	Keyword* kw = keyword_lookup(text.ptr, str_len(text));
	if (kw != NULL) {
		return make_token(lexer, kw->type, TOKEN_VALUE_NONE);
	}
	return make_token(lexer, TT_IDENT, TOKEN_VALUE_STR(str_copy(text)));
}

static Token lex_number(Lexer* lexer)
{
	while (true) {
		MaybeChar c = lexer_peek(lexer, 0);
		if (!c.present || !char_is_digit(c.value)) {
			break;
		}
		lexer_advance(lexer);
	}

	// doesn't need to null terminate, strtoll will stop at the first non-digit
	str text = lexer_text(lexer);
	long long n = strtoll(text.ptr, NULL, 10);
	return make_token(lexer, TT_NUM, TOKEN_VALUE_NUM(n));
}

static Token lex_pp_keyword(Lexer* lexer)
{
	while (true) {
		MaybeChar c = lexer_peek(lexer, 0);
		if (!c.present || !char_is_letter(c.value)) {
			break;
		}
		lexer_advance(lexer);
	}

	str text = lexer_text(lexer);
	PPKeyword* kw = ppkeyword_lookup(text.ptr, str_len(text));
	if (kw == NULL) {
		return make_token(lexer, TT_ERROR, TOKEN_VALUE_NONE);
	}
	lexer->canLexHeaderName = true;
	return make_token(lexer, kw->type, TOKEN_VALUE_NONE);
}

static bool is_header_end(char c, char headerStart)
{
	if (headerStart == '<') {
		return c == '>';
	}
	if (headerStart == '"') {
		return c == '"';
	}
	UNREACHABLE();
}

static str lex_header_name(Lexer* lexer, char headerStart)
{
	// first quote already consumed
	uint64_t start = lexer->pos;
	uint64_t end;
	bool hadError = false;
	while (true) {
		MaybeChar c = lexer_peek(lexer, 0);
		if (!c.present) {
			hadError = true;
			break;
		}
		if (is_header_end(c.value, headerStart)) {
			end = lexer->pos;
			break;
		}
		if (c.value == '\n') {
			hadError = true;
			// the end was probably forgotten
			break;
		}
		lexer_advance(lexer);
	}
	// skip the last quote
	lexer_advance(lexer);
	if (hadError) {
		return str_empty;
	}
	str text = str_ref_chars(&lexer->source.ptr[start], end - start);
	return str_copy(text);
}

static void lex(Lexer* lexer)
{
	skip_whitespace(lexer);
	lexer->tokenStartLoc = (SourceLocation) {
		.filename = lexer->filename,
		.line = lexer->line,
		.column = lexer->column,
	};
	lexer->tokenStart = lexer->pos;
	MaybeChar c = lexer_advance(lexer);
	if (!c.present) {
		lexer->lookahead = make_token(lexer, TT_EOF, TOKEN_VALUE_NONE);
		return;
	}
	bool hadError = false;
	switch (c.value) {
	case '{':
		lexer->lookahead = make_token(lexer, TT_LBRACE, TOKEN_VALUE_NONE);
		break;
	case '}':
		lexer->lookahead = make_token(lexer, TT_RBRACE, TOKEN_VALUE_NONE);
		break;
	case '(':
		lexer->lookahead = make_token(lexer, TT_LPAREN, TOKEN_VALUE_NONE);
		break;
	case ')':
		lexer->lookahead = make_token(lexer, TT_RPAREN, TOKEN_VALUE_NONE);
		break;
	case ';':
		lexer->lookahead = make_token(lexer, TT_SEMI, TOKEN_VALUE_NONE);
		break;
	case '#':
		lexer->lookahead = lex_pp_keyword(lexer);
		break;
	case '<':
		if (lexer->canLexHeaderName) {
			lexer->canLexHeaderName = false;
			str headerName = lex_header_name(lexer, c.value);
			if (str_len(headerName) == 0) {
				hadError = true;
			} else {
				lexer->lookahead =
				        make_token(
				                lexer,
				                TT_HEADER_NAME,
				                TOKEN_VALUE_STR(headerName)
				        );
			}
		} else  {
			hadError = true;
		}
		break;
	default:
		if (char_is_letter(c.value)) {
			lexer->lookahead = lex_ident_or_kw(lexer);
		} else if (char_is_digit(c.value)) {
			lexer->lookahead = lex_number(lexer);
		} else {
			hadError = true;
		}
	}
	if (hadError) {
		lexer->lookahead = make_token(lexer, TT_ERROR, TOKEN_VALUE_NONE);
	}
}

Lexer lexer_new(str source, str filename)
{
	Lexer lexer = {
		.source = source,
		.filename = filename,
		.line = 1,
		.column = 1,
		.tokenStartLoc = (SourceLocation)
		{
			.filename = str_ref(filename),
			.line = 1,
			.column = 1,
		},
		.tokenStart = 0,
		.pos = 0,
		.canLexHeaderName = false,
	};
	// sets lookahead token
	lex(&lexer);
	return lexer;
}

Token lexer_first(Lexer* lexer)
{
	return lexer->lookahead;
}

bool lexer_done(Lexer* lexer)
{
	return lexer->lookahead.type == TT_EOF;
}

Token lexer_next(Lexer* lexer)
{
	lex(lexer);
	return lexer->lookahead;
}
