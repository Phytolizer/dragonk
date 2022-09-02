#pragma once

#include "dragon/core/str.h"

#include <stdbool.h>
#include <stdio.h>

#ifndef TEST_ABORT_ON_FAILURE
#define TEST_ABORT_ON_FAILURE 0
#endif

typedef struct {
	uint64_t passed;
	uint64_t failed;
	uint64_t assertions;
} TestState;

#if TEST_ABORT_ON_FAILURE
#define TEST_ABORT() abort()
#else
#define TEST_ABORT()
#endif
#define TEST_ASSERT(state, test, cleanup, ...) \
	do { \
		++(state)->assertions; \
		if (!(test)) { \
			str message = str_fmt(__VA_ARGS__); \
			cleanup; \
			TEST_ABORT(); \
			return message; \
		} \
	} while (false)

#define FAIL(state, cleanup, ...) \
	TEST_ASSERT(state, false, cleanup, __VA_ARGS__)

#define NO_CLEANUP (void)0
#define CLEANUP(x) x

#define RUN_SUITE(state, name, displayname) \
	do { \
		(void)fprintf(stderr, "SUITE " STR_FMT "\n", STR_ARG(displayname)); \
		str_free(displayname); \
		name##_test_suite(state); \
	} while (false)

#define RUN_TEST(state, name, displayname, ...) \
	do { \
		(void)fprintf(stderr, "TEST  " STR_FMT "\n", STR_ARG(displayname)); \
		str message = name##_test(state, __VA_ARGS__); \
		if (str_len(message) > 0) { \
			++(state)->failed; \
			(void)fprintf( \
			               stderr, \
			               "FAIL  " STR_FMT ": " STR_FMT "\n", \
			               STR_ARG(displayname), \
			               STR_ARG(message) \
			             ); \
			str_free(message); \
		} else { \
			++(state)->passed; \
		} \
		str_free(displayname); \
	} while (false)

#define RUN_SUBTEST(state, name, cleanup, ...) \
	do { \
		str message = name##_subtest(state, __VA_ARGS__); \
		if (str_len(message) > 0) { \
			cleanup; \
			return message; \
		} \
	} while (false)

#define SUITE_FUNC(state, name) \
	void name##_test_suite(TestState* state)

#define TEST_FUNC(state, name, ...) \
	str name##_test(TestState* state __VA_OPT__(,) __VA_ARGS__)

#define SUBTEST_FUNC(state, name, ...) \
	str name##_subtest(TestState* state __VA_OPT__(,) __VA_ARGS__)

#define PASS() \
	return str_empty
