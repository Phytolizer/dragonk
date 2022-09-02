#include "dragon/codegen.h"
#include "embedded/header.nasm.h"

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

static void codegen_expr(FILE* fp, Expression expr)
{
	(void)fprintf(fp, "    push %" PRId64 "\n", expr.number);
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
