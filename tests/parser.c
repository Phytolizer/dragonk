#include "dragon/test/parser.h"

#include <stdint.h>

#include "dragon/core/buf.h"
#include "dragon/core/file.h"
#include "dragon/core/str.h"
#include "dragon/parser.h"
#include "dragon/test/info.h"
#include "dragon/test/list.h"

static TEST_FUNC(state, parse, str path)
{
	SlurpFileResult sourceResult = slurp_file(path);
	if (!sourceResult.ok) {
		FAIL(
		        state,
		        CLEANUP(str_free(sourceResult.get.error)),
		        STR_FMT,
		        STR_ARG(sourceResult.get.error)
		);
	}

	Parser parser = parser_new(sourceResult.get.value, str_ref(path));
	ProgramResult result = parser_parse(&parser);
	parser_free(parser);
	str_free(sourceResult.get.value);
	if (!result.ok) {
		FAIL(
		        state,
		        CLEANUP(str_free(result.get.error)),
		        "parse failed: " STR_FMT,
		        STR_ARG(result.get.error)
		);
	}

	PASS();
}

SUITE_FUNC(state, parser)
{
	StrBuf tests = get_tests(IMPLEMENTED_STAGES);

	for (uint64_t i = 0; i < tests.len; i++) {
		RUN_TEST(state, parse, str_ref(tests.ptr[i]), str_ref(tests.ptr[i]));
		str_free(tests.ptr[i]);
	}
	BUF_FREE(tests);
}
