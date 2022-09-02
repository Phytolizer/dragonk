#include "dragon/codegen.h"
#include "embedded/header.nasm.h"

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

static void codegen_constant_expr(FILE* fp, ConstantExpression* expr)
{
	(void)fprintf(fp, "    push %" PRId64 "\n", expr->number);
}

static void codegen_expr(FILE* fp, Expression* expr);

static void codegen_unary_op_expr(FILE* fp, UnaryOpExpression* expr)
{
	codegen_expr(fp, expr->operand);
	switch (expr->kind) {
	case UNARY_OP_KIND_ARITHMETIC_NEGATION:
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    neg rax\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case UNARY_OP_KIND_BITWISE_NEGATION:
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    not rax\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case UNARY_OP_KIND_LOGICAL_NEGATION:
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, 0\n");
		(void)fprintf(fp, "    sete al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	}
}

static void codegen_expr(FILE* fp, Expression* expr)
{
	switch (expr->type) {
	case EXPRESSION_TYPE_CONSTANT:
		codegen_constant_expr(fp, (ConstantExpression*)expr);
		break;
	case EXPRESSION_TYPE_UNARY_OP:
		codegen_unary_op_expr(fp, (UnaryOpExpression*)expr);
		break;
	}
}

static void codegen_stmt(FILE* fp, Statement stmt)
{
	codegen_expr(fp, stmt.expression);
	(void)fprintf(fp, "    pop rax\n");
}

static void codegen_func(FILE* fp, Function func)
{
	(void)fprintf(fp, STR_FMT ":\n", STR_ARG(func.name));
	codegen_stmt(fp, func.statement);
	(void)fprintf(fp, "    ret\n");
}

void codegen_program(Program program, str outPath)
{
	FILE* fp = fopen(outPath.ptr, "w");
	(void)fwrite(HEADER_NASM, 1, sizeof(HEADER_NASM), fp);

	codegen_func(fp, program.function);

	(void)fclose(fp);
}
