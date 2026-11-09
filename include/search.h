#ifndef MINISEARCH_SEARCH_H
#define MINISEARCH_SEARCH_H

#include "index.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SEARCH_AND = 1,
    SEARCH_OR = 2
} SearchMode;

typedef struct {
    size_t doc_id;
    double score;
    size_t matched_terms;
} SearchResult;

typedef struct {
    SearchResult *items;
    size_t count;
} SearchResults;

bool search_query(const SearchIndex *index,
                  const char *query,
                  SearchMode mode,
                  bool exact_phrase,
                  SearchResults *results,
                  char *error_buffer,
                  size_t error_buffer_size);
void search_results_destroy(SearchResults *results);
bool search_result_has_term(const SearchIndex *index, size_t doc_id, const char *term);

#endif
