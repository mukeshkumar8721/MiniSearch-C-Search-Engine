#include "document.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

static bool path_list_append(DocumentPathList *paths, const char *path)
{
    size_t i;
    char **grown;
    size_t new_capacity;
    char *copy;

    for (i = 0U; i < paths->count; ++i) {
        if (strcmp(paths->items[i], path) == 0) {
            return true;
        }
    }

    if (paths->count == paths->capacity) {
        new_capacity = paths->capacity == 0U ? 8U : paths->capacity * 2U;
        if (new_capacity < paths->capacity ||
            new_capacity > ((size_t)-1) / sizeof(*paths->items)) {
            return false;
        }
        grown = (char **)realloc(paths->items, new_capacity * sizeof(*paths->items));
        if (grown == NULL) {
            return false;
        }
        paths->items = grown;
        paths->capacity = new_capacity;
    }

    copy = ms_strdup(path);
    if (copy == NULL) {
        return false;
    }
    paths->items[paths->count++] = copy;
    return true;
}

static int compare_paths(const void *left, const void *right)
{
    const char *const *a = (const char *const *)left;
    const char *const *b = (const char *const *)right;
    return strcmp(*a, *b);
}

static bool has_txt_extension(const char *name)
{
    size_t length = strlen(name);
    if (length < 4U) {
        return false;
    }
    return name[length - 4U] == '.' &&
           tolower((unsigned char)name[length - 3U]) == 't' &&
           tolower((unsigned char)name[length - 2U]) == 'x' &&
           tolower((unsigned char)name[length - 1U]) == 't';
}

static char *join_path(const char *directory, const char *name)
{
    size_t directory_length = strlen(directory);
    size_t name_length = strlen(name);
    bool has_separator = directory_length > 0U &&
        (directory[directory_length - 1U] == '/' || directory[directory_length - 1U] == '\\');
    size_t total = directory_length + name_length + (has_separator ? 1U : 2U);
    char *path;

    if (total < directory_length || total < name_length) {
        return NULL;
    }
    path = (char *)malloc(total);
    if (path == NULL) {
        return NULL;
    }
    (void)snprintf(path, total, "%s%s%s", directory, has_separator ? "" : "/", name);
    return path;
}

bool document_collect_txt_files(const char *directory,
                                DocumentPathList *paths,
                                char *error_buffer,
                                size_t error_buffer_size)
{
    if (directory == NULL || paths == NULL || directory[0] == '\0') {
        ms_set_error(error_buffer, error_buffer_size, "A corpus directory is required.");
        return false;
    }
    paths->items = NULL;
    paths->count = 0U;
    paths->capacity = 0U;

#ifdef _WIN32
    {
        char *pattern = join_path(directory, "*.txt");
        WIN32_FIND_DATAA data;
        HANDLE handle;
        if (pattern == NULL) {
            ms_set_error(error_buffer, error_buffer_size, "Out of memory while building directory pattern.");
            return false;
        }
        handle = FindFirstFileA(pattern, &data);
        free(pattern);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD code = GetLastError();
            if (code == ERROR_FILE_NOT_FOUND) {
                return true;
            }
            ms_set_error(error_buffer, error_buffer_size, "Cannot read corpus directory '%s' (Windows error %lu).",
                         directory, (unsigned long)code);
            return false;
        }
        do {
            char *path;
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U ||
                !has_txt_extension(data.cFileName)) {
                continue;
            }
            path = join_path(directory, data.cFileName);
            if (path == NULL || !path_list_append(paths, path)) {
                free(path);
                FindClose(handle);
                document_path_list_destroy(paths);
                ms_set_error(error_buffer, error_buffer_size, "Out of memory while listing corpus files.");
                return false;
            }
            free(path);
        } while (FindNextFileA(handle, &data) != 0);
        FindClose(handle);
    }
#else
    {
        DIR *dir = opendir(directory);
        struct dirent *entry;
        if (dir == NULL) {
            ms_set_error(error_buffer, error_buffer_size, "Cannot open corpus directory '%s'.", directory);
            return false;
        }
        while ((entry = readdir(dir)) != NULL) {
            struct stat info;
            char *path;
            if (entry->d_name[0] == '.' || !has_txt_extension(entry->d_name)) {
                continue;
            }
            path = join_path(directory, entry->d_name);
            if (path == NULL) {
                (void)closedir(dir);
                document_path_list_destroy(paths);
                ms_set_error(error_buffer, error_buffer_size, "Out of memory while building a document path.");
                return false;
            }
            if (stat(path, &info) == 0 && S_ISREG(info.st_mode)) {
                if (!path_list_append(paths, path)) {
                    free(path);
                    (void)closedir(dir);
                    document_path_list_destroy(paths);
                    ms_set_error(error_buffer, error_buffer_size, "Out of memory while listing corpus files.");
                    return false;
                }
            }
            free(path);
        }
        (void)closedir(dir);
    }
#endif

    if (paths->count > 1U) {
        qsort(paths->items, paths->count, sizeof(*paths->items), compare_paths);
    }
    return true;
}

void document_path_list_destroy(DocumentPathList *paths)
{
    size_t i;
    if (paths == NULL) {
        return;
    }
    for (i = 0U; i < paths->count; ++i) {
        free(paths->items[i]);
    }
    free(paths->items);
    paths->items = NULL;
    paths->count = 0U;
    paths->capacity = 0U;
}

char *document_basename_copy(const char *path)
{
    const char *base;
    const char *slash;
    const char *backslash;

    if (path == NULL) {
        return NULL;
    }
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    base = path;
    if (slash != NULL && slash + 1 > base) {
        base = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base) {
        base = backslash + 1;
    }
    return ms_strdup(base);
}
