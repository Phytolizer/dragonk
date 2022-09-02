#include "dragon/ast.h"

void program_free(Program program)
{
	str_free(program.header.name);
	str_free(program.function.name);
}
