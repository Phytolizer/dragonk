#include "dragon/test/execute.h"

#include "dragon/core/process.h"
#include "dragon/driver/run.h"
#include "dragon/test/info.h"
#include "dragon/test/list.h"

static TEST_FUNC(state, execute, str testPath)
{
	char* args[] = { "dragon", "-o", "dragon.out", (char*)testPath.ptr };
	int res = run((CArgBuf)BUF_ARRAY(args));
	TEST_ASSERT(
	        state,
	        res == 0,
	        CLEANUP((void)remove("dragon.out")),
	        "dragon failed to compile with exit code %d",
	        res
	);

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
	StrBuf tests = get_tests(IMPLEMENTED_STAGES);
	for (uint64_t i = 0; i < tests.len; i++) {
		RUN_TEST(state, execute, str_ref(tests.ptr[i]), str_ref(tests.ptr[i]));
		str_free(tests.ptr[i]);
	}
	BUF_FREE(tests);
}
