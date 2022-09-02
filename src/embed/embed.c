#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

typedef struct {
  char* ptr;
  size_t len;
} str;

static str str_ref(const char* ptr) {
  return (str){
      .ptr = (char*)ptr,
      .len = strlen(ptr),
  };
}

static void str_free(str s) {
  free(s.ptr);
}

typedef struct {
  bool ok;
  str error;
  str value;
} str_result_t;

typedef struct {
#ifdef _WIN32
  HANDLE handle;
#else
  int fd;
#endif
  bool valid;
} platform_file;

static platform_file platform_fopen(str path) {
#ifdef _WIN32
  HANDLE handle = CreateFileA(path.ptr,
      GENERIC_READ,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
  return (platform_file){
      .handle = handle,
      .valid = handle != INVALID_HANDLE_VALUE,
  };
#else
  int fd = open(path.ptr, O_RDONLY);
  return (platform_file){.fd = fd, .valid = fd != -1};
#endif
}

static void platform_fclose(platform_file file) {
  if (file.valid) {
#ifdef _WIN32
    CloseHandle(file.handle);
#else
    close(file.fd);
#endif
  }
}

static size_t get_filelen(platform_file f) {
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

static size_t platform_fread(platform_file f, char* buf, size_t len) {
#ifdef _WIN32
  DWORD read;
  ReadFile(f.handle, buf, (DWORD)len, &read, NULL);
  return (size_t)read;
#else
  return (size_t)read(f.fd, buf, len);
#endif
}

static str_result_t slurp_file(str path) {
  platform_file f = platform_fopen(path);
  if (!f.valid) {
    return (str_result_t){
        .ok = false,
        .error = str_ref("failed to open file"),
    };
  }

  size_t len = get_filelen(f);

  char* buf = malloc(len + 1);
  if (buf == NULL) {
    return (str_result_t){
        .ok = false,
        .error = str_ref("failed to allocate buffer"),
    };
  }

  size_t read = platform_fread(f, buf, len);
  if (read != len) {
    return (str_result_t){
        .ok = false,
        .error = str_ref("failed to read file"),
    };
  }

  buf[len] = '\0';

  return (str_result_t){
      .ok = true,
      .value =
          (str){
              .ptr = buf,
              .len = len,
          },
  };
}

int main(int argc, char** argv) {
  if (argc < 5) {
    printf("Usage: %s <input> <header> <output> <name>\n", argv[0]);
    return 1;
  }

  str_result_t result = slurp_file(str_ref(argv[1]));
  if (!result.ok) {
    printf("%s\n", result.error.ptr);
    str_free(result.error);
    return 1;
  }

  FILE* header = fopen(argv[2], "w");
  if (header == NULL) {
    printf("Could not open %s for writing\n", argv[2]);
    str_free(result.value);
    return 1;
  }

  (void)fprintf(header, "#pragma once\n\n");
  (void)fprintf(
      header, "extern const char %s[%zu];\n", argv[4], result.value.len);
  (void)fclose(header);

  FILE* source = fopen(argv[3], "w");
  if (source == NULL) {
    printf("Could not open output file\n");
    str_free(result.value);
    return 1;
  }
  (void)fprintf(source, "#include \"%s\"\n\n", argv[2]);
  (void)fprintf(source, "const char %s[%zu] = {\n", argv[4], result.value.len);
  for (size_t i = 0; i < result.value.len; i++) {
    (void)fprintf(source, "  %#x,\n", result.value.ptr[i]);
  }
  (void)fprintf(source, "};\n");
  (void)fclose(source);
  str_free(result.value);
}
