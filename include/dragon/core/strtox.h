#pragma once

#include <stdint.h>

#include "dragon/core/str.h"

#define STRTOX_RESULT(T) \
	struct { \
		T value; \
		const char* endptr; \
		int err; \
	}

typedef STRTOX_RESULT(int64_t) Str2I64Result;

Str2I64Result str2i64(str s, int base);
