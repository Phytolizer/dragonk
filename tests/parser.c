#include "dragon/test/parser.h"

#include <stdbool.h>
#include <stdint.h>

#include "dragon/ast.h"
#include "dragon/core/buf.h"
#include "dragon/core/file.h"
#include "dragon/core/str.h"
#include "dragon/parser.h"
#include "dragon/test/info.h"
#include "dragon/test/list.h"

static TEST_FUNC(state, parse, str path, bool isValid, bool skipOnFailure)
{
	SlurpFileResult sourceResult = slurp_file(path);
	TEST_ASSERT(
	        state,
	        sourceResult.ok,
	        CLEANUP(str_free(sourceResult.get.error)),
	        STR_FMT,
	        STR_ARG(sourceResult.get.error)
	);

	Parser parser = parser_new(sourceResult.get.value, str_ref(path));
	ProgramResult result = parser_parse(&parser);
	parser_free(parser);
	str_free(sourceResult.get.value);
	if (isValid) {
		if (skipOnFailure) {
			if (result.ok) {
				program_free(result.get.value);
			} else {
				str_free(result.get.error);
			}
			SKIP();
		}
		TEST_ASSERT(
		        state,
		        result.ok,
		        CLEANUP(str_free(result.get.error)),
		        "parse failed: " STR_FMT,
		        STR_ARG(result.get.error)
		);
	} else {
		TEST_ASSERT(
		        state,
		        !result.ok,
		        CLEANUP(program_free(result.get.value)),
		        "parse succeeded on invalid input!"
		);
		str_free(result.get.error);
		PASS();
	}

	program_free(result.get.value);

	PASS();
}

SUITE_FUNC(state, parser)
{
	TestCaseBuf tests = get_tests(IMPLEMENTED_STAGES);

	for (uint64_t i = 0; i < tests.len; i++) {
		TestCase test = tests.ptr[i];
		RUN_TEST(
		        state,
		        parse,
		        str_fmt("parsing " STR_FMT, STR_ARG(test.path)),
		        str_ref(test.path),
		        test.isValid,
		        test.skipOnFailure
		);
		str_free(test.path);
	}
	BUF_FREE(tests);
}
