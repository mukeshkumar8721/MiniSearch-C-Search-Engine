#ifndef MINISEARCH_STORAGE_H
#define MINISEARCH_STORAGE_H

#include "index.h"
#include <stdbool.h>

bool storage_save(const SearchIndex *index,
                  const char *file_path,
                  char *error_buffer,
                  size_t error_buffer_size);
bool storage_load(SearchIndex *target,
                  const char *file_path,
                  char *error_buffer,
                  size_t error_buffer_size);

#endif
