#include "dragon/test/lexer.h"

#include <stdint.h>

#include "dragon/core/buf.h"
#include "dragon/core/file.h"
#include "dragon/core/str.h"
#include "dragon/lexer.h"
#include "dragon/test/info.h"
#include "dragon/test/list.h"
#include "dragon/token.h"

static TEST_FUNC(state, lex, str path, bool skipOnFailure)
{
	SlurpFileResult sourceResult = slurp_file(path);
	TEST_ASSERT(
	        state,
	        sourceResult.ok,
	        CLEANUP(str_free(sourceResult.get.error)),
	        STR_FMT,
	        STR_ARG(sourceResult.get.error)
	);

	Lexer lexer = lexer_new(sourceResult.get.value, str_ref(path));
	for (Token tok = lexer_first(&lexer); !lexer_done(&lexer); tok = lexer_next(&lexer)) {
		if (tok.type == TT_ERROR) {
			if (skipOnFailure) {
				token_free(tok);
				str_free(sourceResult.get.value);
				SKIP();
			}
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
	TestCaseBuf tests = get_tests(IMPLEMENTED_STAGES);

	for (uint64_t i = 0; i < tests.len; i++) {
		TestCase test = tests.ptr[i];
		RUN_TEST(
		        state,
		        lex,
		        str_fmt("lexing " STR_FMT, STR_ARG(test.path)),
		        str_ref(test.path),
		        test.skipOnFailure
		);
		str_free(test.path);
	}
	BUF_FREE(tests);
}
