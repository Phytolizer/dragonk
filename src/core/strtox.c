#include "dragon/core/strtox.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

static bool char_is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static char char_to_upper(char c)
{
	if (c >= 'a' && c <= 'z') {
		return (char)(c - 'a' + 'A');
	}
	return c;
}

static bool char_is_letter(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

#define MKTAB(f) \
	{ \
		f(2), f(3), f(4), f(5), f(6), f(7), f(8), f(9), f(10), f(11), f(12), f(13), \
		f(14), f(15), f(16), f(17), f(18), f(19), f(20), f(21), f(22), f(23), f(24), \
		f(25), f(26), f(27), f(28), f(29), f(30), f(31), f(32), f(33), f(34), f(35), f(36) \
	}

static int64_t str2i64_cutoff(int i)
{
	return INT64_MAX / i;
}

static int64_t str2i64_cutlim(int i)
{
	return INT64_MAX % i;
}

Str2I64Result str2i64(str in, int base)
{
	const int64_t cutoff_tab[] = MKTAB(str2i64_cutoff);
	const int64_t cutlim_tab[] = MKTAB(str2i64_cutlim);

	if (base < 0 || base == 1 || base > 36) {
		return (Str2I64Result) {.err = EINVAL};
	}

	const char* save = in.ptr;
	const char* s = save;
	while (char_is_space(*s)) {
		s++;
	}
	if (*s == '\0') {
		// no number
		return (Str2I64Result) { .endptr = in.ptr };
	}

	bool negative = false;
	if (*s == '-') {
		negative = true;
		s++;
	} else if (*s == '+') {
		s++;
	}

	if (*s == '0') {
		if ((base == 0 || base == 16) && char_to_upper(s[1]) == 'X') {
			base = 16;
			s += 2;
		} else if (base == 0) {
			base = 8;
		}
	} else if (base == 0) {
		base = 10;
	}

	save = s;

	const char* end = NULL;
	int64_t cutoff = cutoff_tab[base - 2];
	int64_t cutlim = cutlim_tab[base - 2];
	bool overflow = false;
	char c = *s;

	int64_t i = 0;

	while (c != '\0') {
		if (s == end) {
			break;
		}

		if (c >= '0' && c <= '9') {
			c -= '0';
		} else if (char_is_letter(c)) {
			c = (char)(char_to_upper(c) - 'A' + 10);
		} else {
			break;
		}

		if (c >= base) {
			break;
		}

		if (i > cutoff || (i == cutoff && c > cutlim)) {
			overflow = true;
		} else {
			i *= (int64_t)base;
			i += c;
		}

		s++;
		c = *s;
	}

	if (s == save) {
		// no number
		return (Str2I64Result) { .endptr = in.ptr };
	}

	end = s;

	if (overflow) {
		return (Str2I64Result) { .err = ERANGE };
	}

	return (Str2I64Result) { .value = negative ? -i : i, .endptr = end };
}
