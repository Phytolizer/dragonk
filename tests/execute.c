#include "dragon/test/execute.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "dragon/core/buf.h"
#include "dragon/core/process.h"
#include "dragon/core/str.h"
#include "dragon/driver/run.h"
#include "dragon/test/info.h"
#include "dragon/test/list.h"

static TEST_FUNC(state, execute, str testPath, bool isValid, bool skipOnFailure)
{
	char* args[] = { "dragon", "-o", "dragon.out", (char*)testPath.ptr };
	int outpipe[2];
	int errpipe[2];
	TEST_ASSERT(state, pipe(outpipe) == 0, NO_CLEANUP, "failed to create pipe: %m");
	TEST_ASSERT(state, pipe(errpipe) == 0, NO_CLEANUP, "failed to create pipe: %m");
	FILE* out = fdopen(outpipe[1], "w");
	FILE* err = fdopen(errpipe[1], "w");
	int res = run((CArgBuf)BUF_ARRAY(args), out, err);
	(void)fclose(out);
	(void)fclose(err);
	if (isValid) {
		if (res != 0) {
			if (skipOnFailure) {
				(void)remove("dragon.out");
				SKIP();
			}
			FILE* errIn = fdopen(errpipe[0], "r");
			char* line = NULL;
			size_t len = 0;
			ssize_t nread;
			while ((nread = getline(&line, &len, errIn)) != -1) {
				(void)fprintf(stderr, "%s", line);
			}
			FAIL(
			        state,
			        CLEANUP((void)remove("dragon.out")),
			        "dragon failed to compile"
			);
		}
	} else {
		TEST_ASSERT(
		        state,
		        res != 0,
		        CLEANUP((void)remove("dragon.out")),
		        "dragon compiled invalid test " STR_FMT,
		        STR_ARG(testPath)
		);
		PASS();
	}

	const char* runArgs[] = { "./dragon.out" };
	ProcessCreateResult dragonResult =
	        process_run(
	                (ProcessCStrBuf)BUF_ARRAY(runArgs),
	                PROCESS_OPTION_COMBINED_STDOUT_STDERR
	        );
	TEST_ASSERT(
	        state,
	        dragonResult.present,
	        CLEANUP((void)remove("dragon.out")),
	        "dragon program failed to spawn"
	);

	int dragonCode = dragonResult.value.returnCode;
	process_destroy(&dragonResult.value);
	(void)remove("dragon.out");

	const char* gccArgs[] = { "gcc", testPath.ptr, "-o", "gcc.out" };
	ProcessCreateResult gccResult =
	        process_run(
	                (ProcessCStrBuf)BUF_ARRAY(gccArgs),
	                PROCESS_OPTION_COMBINED_STDOUT_STDERR | PROCESS_OPTION_SEARCH_USER_PATH
	        );
	TEST_ASSERT(
	        state,
	        gccResult.present,
	        CLEANUP((void)remove("dragon.out"); (void)remove("gcc.out")),
	        "gcc failed to compile"
	);
	process_destroy(&gccResult.value);

	const char* gccRunArgs[] = { "./gcc.out" };
	ProcessCreateResult gccRunResult =
	        process_run(
	                (ProcessCStrBuf)BUF_ARRAY(gccRunArgs),
	                PROCESS_OPTION_COMBINED_STDOUT_STDERR
	        );
	TEST_ASSERT(
	        state,
	        gccRunResult.present,
	        CLEANUP((void)remove("gcc.out")),
	        "gcc program failed to spawn"
	);

	int gccCode = gccRunResult.value.returnCode;
	process_destroy(&gccRunResult.value);

	(void)remove("gcc.out");

	TEST_ASSERT(
	        state,
	        dragonCode == gccCode,
	        NO_CLEANUP,
	        "dragon and gcc produced different exit codes: %d vs %d",
	        dragonCode,
	        gccCode
	);

	PASS();
}

SUITE_FUNC(state, execute)
{
	TestCaseBuf tests = get_tests(IMPLEMENTED_STAGES);
	for (uint64_t i = 0; i < tests.len; i++) {
		TestCase test = tests.ptr[i];
		RUN_TEST(
		        state,
		        execute,
		        str_ref(test.path),
		        str_ref(test.path),
		        test.isValid,
		        test.skipOnFailure
		);
		str_free(test.path);
	}
	BUF_FREE(tests);
}
