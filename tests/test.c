#include <inttypes.h>
#include <stdio.h>

#include "dragon/core/str.h"
#include "dragon/test/lexer.h"
#include "dragon/test/parser.h"
#include "dragon/test/test.h"

static void run_all(TestState* state)
{
	RUN_SUITE(state, lexer, str_lit("lexer"));
	RUN_SUITE(state, parser, str_lit("parser"));
}

int main(void)
{
	TestState state = {0};
	run_all(&state);
	printf(
	        "passed: %" PRIu64 ", failed: %" PRIu64 ", assertions: %" PRIu64 "\n",
	        state.passed,
	        state.failed,
	        state.assertions
	);
	return state.failed > 0;
}
