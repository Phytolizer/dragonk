#include "dragon/core/str.h"

#include <stdlib.h>
#include <string.h>

void str_free(str s)
{
	if (str_is_owner(s)) {
		free((char*)s.ptr);
	}
}

str z_str_ref_str(str s)
{
	return (str) {
		.ptr = s.ptr, .info = z_str_ref_info(str_len(s))
	};
}

str z_str_ref_chars(const char* ptr)
{
	return (str) {
		.ptr = ptr, .info = z_str_ref_info(strlen(ptr))
	};
}

str str_acquire(const char* ptr, uint64_t len)
{
	if (ptr == NULL) {
		return str_empty;
	}

	if (len == 0) {
		free((char*)ptr);
		return str_empty;
	}

	return (str) {
		.ptr = ptr, .info = z_str_owner_info(len)
	};
}

str str_ref_chars(const char* ptr, uint64_t len)
{
	if (ptr == NULL) {
		return str_empty;
	}

	if (len == 0) {
		return str_empty;
	}

	return (str) {
		.ptr = ptr, .info = z_str_ref_info(len)
	};
}

str str_copy(str s)
{
	if (str_is_empty(s)) {
		return str_empty;
	}

	char* ptr = malloc(str_len(s) + 1);
	if (ptr == NULL) {
		return str_empty;
	}

	memcpy(ptr, str_ptr(s), str_len(s));
	ptr[str_len(s)] = '\0';

	return (str) {
		.ptr = ptr, .info = z_str_owner_info(str_len(s))
	};
}
