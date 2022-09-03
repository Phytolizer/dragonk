#include "dragon/test/list.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "dragon/config.h"
#include "dragon/core/buf.h"
#include "dragon/core/strtox.h"

typedef struct {
	bool isValidDir;
	bool isInvalidDir;
} TestDirInfo;

static void walk_stage_dir(TestCaseBuf* buf, TestDirInfo* info, str path)
{
	DIR* dir = opendir(path.ptr);
	if (dir == NULL) {
		return;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		str name = str_ref(entry->d_name);
		if (str_eq(name, str_lit(".")) || str_eq(name, str_lit(".."))) {
			continue;
		}

		str childPath = path_join(path, name);
		if (entry->d_type == DT_DIR) {
			if (str_eq(name, str_lit("invalid"))) {
				info->isInvalidDir = true;
			} else if (str_eq(name, str_lit("valid"))) {
				info->isValidDir = true;
			}
			walk_stage_dir(buf, info, str_ref(childPath));
			str_free(childPath);
		} else if (entry->d_type == DT_REG) {
			if (str_endswith(name, str_lit(".c"))) {
				TestCase testCase;
				testCase.path = childPath;
				testCase.isValid = info->isValidDir;
				testCase.skipOnFailure =
				        str_startswith(name, str_lit("skip_on_failure_"));
				BUF_PUSH(buf, testCase);
			}
		}
	}

	closedir(dir);
}

TestCaseBuf get_tests(uint64_t maxStage)
{
	TestCaseBuf result = BUF_NEW;

	str top = str_lit(CMAKE_TOPDIR "/tests/cases");

	DIR* topdir = opendir(top.ptr);
	if (topdir == NULL) {
		(void)fprintf(stderr, "failed to open tests directory: " STR_FMT "\n", STR_ARG(top));
		exit(1);
	}

	const str prefix = str_lit("stage");

	struct dirent* entry;
	while ((entry = readdir(topdir)) != NULL) {
		if (entry->d_type != DT_DIR) {
			continue;
		}
		str name = str_ref(entry->d_name);
		if (!str_startswith(name, prefix)) {
			continue;
		}
		str namep = str_shifted(name, str_len(prefix));
		Str2I64Result stage = str2i64(namep, 10);
		if (stage.err != 0) {
			continue;
		}
		if (stage.value <= maxStage) {
			str stagePath = path_join(top, name);
			TestDirInfo info = {0};
			walk_stage_dir(&result, &info, str_ref(stagePath));
			str_free(stagePath);
		}
	}

	closedir(topdir);

	return result;
}
