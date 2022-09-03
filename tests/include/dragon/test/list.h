#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dragon/core/buf.h"
#include "dragon/core/str.h"

typedef struct {
	str path;
	bool isValid;
	bool skipOnFailure;
} TestCase;

typedef BUF(TestCase) TestCaseBuf;

TestCaseBuf get_tests(uint64_t maxStage);
