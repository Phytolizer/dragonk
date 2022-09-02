#include "dragon/test/list.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "dragon/config.h"
#include "dragon/core/buf.h"
#include "dragon/core/strtox.h"

#define path_join(...) str_join(str_lit("/"), (StrBuf)BUF_ARRAY(((str[]) {__VA_ARGS__})))

StrBuf get_tests(uint64_t maxStage)
{
	StrBuf result = BUF_NEW;

	str top = str_lit(CMAKE_TOPDIR "/tests");

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

			DIR* stageDir = opendir(stagePath.ptr);
			if (stageDir == NULL) {
				(void)fprintf(
				        stderr,
				        "failed to open stage directory: " STR_FMT "\n",
				        STR_ARG(stagePath)
				);
				exit(1);
			}

			struct dirent* stageEntry;
			while ((stageEntry = readdir(stageDir)) != NULL) {
				if (stageEntry->d_type != DT_REG) {
					continue;
				}
				str testName = str_ref(stageEntry->d_name);
				if (!str_endswith(testName, str_lit(".c"))) {
					continue;
				}
				str testPath = path_join(str_ref(stagePath), testName);
				BUF_PUSH(&result, testPath);
			}

			closedir(stageDir);
			str_free(stagePath);
		}
	}

	closedir(topdir);

	return result;
}
