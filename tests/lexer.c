#include "dragon/test/lexer.h"
#include "dragon/core/file.h"
#include "dragon/lexer.h"
#include "dragon/test/info.h"
#include "dragon/test/list.h"

static TEST_FUNC(state, lex, str path)
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

	Lexer lexer = lexer_new(sourceResult.get.value, str_ref(path));
	for (Token tok = lexer_first(&lexer); !lexer_done(&lexer); tok = lexer_next(&lexer)) {
		if (tok.type == TT_ERROR) {
			FAIL(
			        state,
			        CLEANUP(token_free(tok); str_free(sourceResult.get.value)),
			        "lex failed on " SOURCE_LOCATION_FMT ": '" STR_FMT "'",
			        SOURCE_LOCATION_ARG(tok.location),
			        STR_ARG(tok.text)
			);
		}
		token_free(tok);
	}

	str_free(sourceResult.get.value);

	PASS();
}

SUITE_FUNC(state, lexer)
{
	StrBuf tests = get_tests(IMPLEMENTED_STAGES);

	for (uint64_t i = 0; i < tests.len; i++) {
		RUN_TEST(state, lex, str_ref(tests.ptr[i]), str_ref(tests.ptr[i]));
		str_free(tests.ptr[i]);
	}
	BUF_FREE(tests);
}