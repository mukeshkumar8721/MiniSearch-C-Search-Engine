#ifndef MINISEARCH_DOCUMENT_H
#define MINISEARCH_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>

/* A list of candidate .txt paths found directly inside one corpus directory. */
typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} DocumentPathList;

bool document_collect_txt_files(const char *directory,
                                DocumentPathList *paths,
                                char *error_buffer,
                                size_t error_buffer_size);
void document_path_list_destroy(DocumentPathList *paths);
char *document_basename_copy(const char *path);

#endif
