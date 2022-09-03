#include "dragon/codegen.h"

#include <inttypes.h>
#include <stdio.h>

#include "embedded/header.nasm.h"

typedef struct {
	FILE* fp;
	uint64_t labelCount;
} Compiler;

static str get_label(Compiler* compiler)
{
	return str_fmt(".L%" PRIu64, compiler->labelCount++);
}

static void codegen_constant_expr(Compiler* compiler, ConstantExpression* expr)
{
	(void)fprintf(compiler->fp, "    push %" PRId64 "\n", expr->number);
}

static void codegen_expr(Compiler* compiler, Expression* expr);

static void codegen_unary_op_expr(Compiler* compiler, UnaryOpExpression* expr)
{
	FILE* fp = compiler->fp;
	codegen_expr(compiler, expr->operand);
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

static void codegen_binary_op_expr(Compiler* compiler, BinaryOpExpression* expr)
{
	FILE* fp = compiler->fp;
	switch (expr->kind) {
	case BINARY_OP_KIND_ADDITION:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    add rax, rdi\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_SUBTRACTION:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    sub rax, rdi\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_MULTIPLICATION:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    imul rax, rdi\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_DIVISION:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cqo\n");
		(void)fprintf(fp, "    idiv rdi\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_LOGICAL_AND: {
		codegen_expr(compiler, expr->left);
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, 0\n");
		str trueLabel = get_label(compiler);
		(void)fprintf(fp, "    jne " STR_FMT "\n", STR_ARG(trueLabel));
		str endLabel = get_label(compiler);
		(void)fprintf(fp, "    jmp " STR_FMT "\n", STR_ARG(endLabel));
		(void)fprintf(fp, STR_FMT ":\n", STR_ARG(trueLabel));
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, 0\n");
		(void)fprintf(fp, "    setne al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, STR_FMT ":\n", STR_ARG(endLabel));
		(void)fprintf(fp, "    push rax\n");
		str_free(trueLabel);
		str_free(endLabel);
		break;
	}
	case BINARY_OP_KIND_LOGICAL_OR: {
		codegen_expr(compiler, expr->left);
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, 0\n");
		str falseLabel = get_label(compiler);
		(void)fprintf(fp, "    je " STR_FMT "\n", STR_ARG(falseLabel));
		(void)fprintf(fp, "    mov rax, 1\n");
		str endLabel = get_label(compiler);
		(void)fprintf(fp, "    jmp " STR_FMT "\n", STR_ARG(endLabel));
		(void)fprintf(fp, STR_FMT ":\n", STR_ARG(falseLabel));
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, 0\n");
		(void)fprintf(fp, "    setne al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, STR_FMT ":\n", STR_ARG(endLabel));
		(void)fprintf(fp, "    push rax\n");
		str_free(falseLabel);
		str_free(endLabel);
		break;
	}
	case BINARY_OP_KIND_LESS:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, rdi\n");
		(void)fprintf(fp, "    setl al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_LESS_EQUAL:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, rdi\n");
		(void)fprintf(fp, "    setle al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_GREATER:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, rdi\n");
		(void)fprintf(fp, "    setg al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_GREATER_EQUAL:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, rdi\n");
		(void)fprintf(fp, "    setge al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_EQUALITY:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, rdi\n");
		(void)fprintf(fp, "    sete al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	case BINARY_OP_KIND_INEQUALITY:
		codegen_expr(compiler, expr->left);
		codegen_expr(compiler, expr->right);
		(void)fprintf(fp, "    pop rdi\n");
		(void)fprintf(fp, "    pop rax\n");
		(void)fprintf(fp, "    cmp rax, rdi\n");
		(void)fprintf(fp, "    setne al\n");
		(void)fprintf(fp, "    movzx rax, al\n");
		(void)fprintf(fp, "    push rax\n");
		break;
	}
}

static void codegen_expr(Compiler* compiler, Expression* expr)
{
	switch (expr->type) {
	case EXPRESSION_TYPE_CONSTANT:
		codegen_constant_expr(compiler, (ConstantExpression*)expr);
		break;
	case EXPRESSION_TYPE_UNARY_OP:
		codegen_unary_op_expr(compiler, (UnaryOpExpression*)expr);
		break;
	case EXPRESSION_TYPE_BINARY_OP:
		codegen_binary_op_expr(compiler, (BinaryOpExpression*)expr);
		break;
	}
}

static void codegen_stmt(Compiler* compiler, Statement stmt)
{
	codegen_expr(compiler, stmt.expression);
	(void)fprintf(compiler->fp, "    pop rax\n");
}

static void codegen_func(Compiler* compiler, Function func)
{
	FILE* fp = compiler->fp;
	(void)fprintf(fp, STR_FMT ":\n", STR_ARG(func.name));
	codegen_stmt(compiler, func.statement);
	(void)fprintf(fp, "    ret\n");
}

void codegen_program(Program program, str outPath)
{
	FILE* fp = fopen(outPath.ptr, "w");
	Compiler compiler = { .fp = fp };
	(void)fwrite(HEADER_NASM, 1, sizeof(HEADER_NASM), fp);

	codegen_func(&compiler, program.function);

	(void)fclose(fp);
}
