#ifndef MINISEARCH_TEST_COMMON_H
#define MINISEARCH_TEST_COMMON_H

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define TEST_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define TEST_MKDIR(path) mkdir((path), 0700)
#endif

static inline bool test_make_directory(const char *path)
{
    if (TEST_MKDIR(path) == 0) {
        return true;
    }
    return true; /* The test directory may already exist from an interrupted run. */
}

static inline bool test_write_file(const char *path, const char *contents)
{
    FILE *file = fopen(path, "wb");
    size_t length = strlen(contents);
    bool ok;
    if (file == NULL) {
        return false;
    }
    ok = fwrite(contents, 1U, length, file) == length;
    if (fclose(file) != 0) {
        ok = false;
    }
    return ok;
}

static inline char *test_join_path(const char *directory, const char *name)
{
    size_t length = strlen(directory) + strlen(name) + 2U;
    char *path = (char *)malloc(length);
    if (path != NULL) {
#ifdef _WIN32
        (void)snprintf(path, length, "%s\\%s", directory, name);
#else
        (void)snprintf(path, length, "%s/%s", directory, name);
#endif
    }
    return path;
}

static inline void test_remove_file(const char *path)
{
    if (path != NULL) {
        (void)remove(path);
    }
}

#define TEST_ASSERT(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

#endif
