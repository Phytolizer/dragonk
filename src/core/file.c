#include "dragon/core/file.h"

#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

typedef struct {
#ifdef _WIN32
	HANDLE handle;
#else
	int fd;
#endif
	bool valid;
} PlatformFile;

static PlatformFile platform_fopen(str path)
{
#ifdef _WIN32
	HANDLE handle = CreateFileA(
	                        path.ptr,
	                        GENERIC_READ,
	                        FILE_SHARE_READ,
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL
	                );
	return (platform_file) {
		.handle = handle,
		.valid = handle != INVALID_HANDLE_VALUE,
	};
#else
	int fd = open(path.ptr, O_RDONLY);
	return (PlatformFile) {.fd = fd, .valid = fd != -1};
#endif
}

static void platform_fclose(PlatformFile file)
{
	if (file.valid) {
#ifdef _WIN32
		CloseHandle(file.handle);
#else
		close(file.fd);
#endif
	}
}

static size_t platform_filelen(PlatformFile f)
{
#ifdef _WIN32
	LARGE_INTEGER size;
	GetFileSizeEx(f.handle, &size);
	return (size_t)size.QuadPart;
#else
	struct stat st;
	fstat(f.fd, &st);
	return (size_t)st.st_size;
#endif
}

static size_t platform_fread(PlatformFile f, char* buf, size_t len)
{
#ifdef _WIN32
	DWORD read;
	ReadFile(f.handle, buf, (DWORD)len, &read, NULL);
	return (size_t)read;
#else
	return (size_t)read(f.fd, buf, len);
#endif
}

SlurpFileResult slurp_file(str filename)
{
	PlatformFile f = platform_fopen(filename);
	if (!f.valid) {
		str msg =
		        str_fmt(
		                "failed to open '" STR_FMT "' for reading",
		                STR_ARG(filename)
		        );
		return (SlurpFileResult)ERR(msg);
	}

	size_t len = platform_filelen(f);

	char* buf = malloc(len + 1);
	if (buf == NULL) {
		platform_fclose(f);
		str msg =
		        str_fmt(
		                "failed to allocate memory for '" STR_FMT "' contents",
		                STR_ARG(filename)
		        );
		return (SlurpFileResult)ERR(msg);
	}

	size_t read = platform_fread(f, buf, len);
	if (read != len) {
		platform_fclose(f);
		free(buf);
		str msg =
		        str_fmt(
		                "failed to read '" STR_FMT "' contents",
		                STR_ARG(filename)
		        );
		return (SlurpFileResult)ERR(msg);
	}

	buf[len] = '\0';

	platform_fclose(f);

	return (SlurpFileResult)OK(str_acquire(buf, len));
}
