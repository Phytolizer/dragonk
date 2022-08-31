#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	const char* ptr;
	uint64_t info;
} str;

#define z_str_ref_info(x) ((x) << 1U)
#define z_str_owner_info(x) (z_str_ref_info(x) | UINT64_C(1))

#define str_empty ((str){NULL, z_str_ref_info(0)})
#define str_lit(s) ((str){.ptr = (s), .info = z_str_ref_info(sizeof(s) - 1)})

static inline uint64_t str_len(str s)
{
	return s.info >> 1U;
}

static inline const char* str_ptr(str s)
{
	if (s.ptr != NULL) {
		return s.ptr;
	}
	return "";
}

static inline const char* str_end(str s)
{
	return str_ptr(s) + str_len(s);
}

static inline bool str_is_empty(str s)
{
	return str_len(s) == 0;
}

static inline bool str_is_owner(str s)
{
	return (s.info & 1U) != 0;
}

static inline bool str_is_ref(str s)
{
	return !str_is_owner(s);
}

void str_free(str s);

#define str_ref(s) \
	_Generic((s), str : z_str_ref_str, const char* : z_str_ref_chars, char* : z_str_ref_chars)(s)

str z_str_ref_str(str s);
str z_str_ref_chars(const char* ptr);
str str_acquire(const char* ptr, uint64_t len);
str str_ref_chars(const char* ptr, uint64_t len);
str str_copy(str s);
