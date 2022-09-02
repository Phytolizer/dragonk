#include "dragon/ast.h"

#include <inttypes.h>

typedef struct {
	uint64_t indent;
	str indentStr;
} AstEmitter;

static const char* UNARY_OP_KIND_STRINGS[] = {
#define X(x) #x,
#include "dragon/unary_op_kinds.def"
#undef X
};

static void emitter_indent(AstEmitter* em)
{
	em->indent++;
	em->indentStr = str_cat(em->indentStr, str_lit("    "));
}

static void emitter_dedent(AstEmitter* em)
{
	em->indent--;
	str tmp = str_copy(str_ref_chars(em->indentStr.ptr, str_len(em->indentStr) - 4));
	str_free(em->indentStr);
	em->indentStr = tmp;
}

static str emit_line(AstEmitter* em, str line)
{
	return str_cat(str_ref(em->indentStr), line, str_lit("\n"));
}

static str expr_to_str(Expression* expr)
{
	switch (expr->type) {
	case EXPRESSION_TYPE_CONSTANT:
		return str_fmt("%" PRId64, ((ConstantExpression*)expr)->number);
	case EXPRESSION_TYPE_UNARY_OP: {
		UnaryOpExpression* unary = (UnaryOpExpression*)expr;
		str s = str_fmt("%s", UNARY_OP_KIND_STRINGS[unary->kind]);
		s = str_cat(s, expr_to_str(unary->operand));
		return s;
	}
	}
}

static str stmt_to_str(Statement stmt)
{
	str s = str_empty;
	s = str_cat(s, str_lit("RETURN INT "), expr_to_str(stmt.expression));
	return s;
}

static str func_to_str(AstEmitter* em, Function func)
{
	str name = str_ref(func.name);
	str s = str_empty;
	s = str_cat(s, emit_line(em, str_fmt("FUN INT " STR_FMT, STR_ARG(name))));
	emitter_indent(em);
	s = str_cat(s, emit_line(em, stmt_to_str(func.statement)));
	emitter_dedent(em);
	return s;
}

str program_to_str(Program program)
{
	str s = str_empty;
	AstEmitter em = {0};
	s = str_cat(s, func_to_str(&em, program.function));
	str_free(em.indentStr);
	return s;
}

void program_free(Program program)
{
	str_free(program.function.name);
}
