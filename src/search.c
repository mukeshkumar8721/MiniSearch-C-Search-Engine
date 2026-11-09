#include "search.h"
#include "ranking.h"
#include "tokenizer.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int compare_search_results(const void *left, const void *right)
{
    const SearchResult *a = (const SearchResult *)left;
    const SearchResult *b = (const SearchResult *)right;
    if (a->score < b->score) {
        return 1;
    }
    if (a->score > b->score) {
        return -1;
    }
    if (a->doc_id < b->doc_id) {
        return -1;
    }
    if (a->doc_id > b->doc_id) {
        return 1;
    }
    return 0;
}

static bool position_exists(const Posting *posting, size_t position)
{
    size_t low = 0U;
    size_t high;
    if (posting == NULL) {
        return false;
    }
    high = posting->positions_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2U;
        if (posting->positions[middle] == position) {
            return true;
        }
        if (posting->positions[middle] < position) {
            low = middle + 1U;
        } else {
            high = middle;
        }
    }
    return false;
}

static bool document_has_phrase(const SearchIndex *index,
                                size_t doc_id,
                                char *const *phrase_terms,
                                size_t phrase_count)
{
    const TermEntry *first_entry;
    const Posting *first_posting;
    size_t i;

    if (phrase_count == 0U) {
        return false;
    }
    first_entry = index_find_term(index, phrase_terms[0]);
    first_posting = index_get_posting(first_entry, doc_id);
    if (first_posting == NULL) {
        return false;
    }

    for (i = 0U; i < first_posting->positions_count; ++i) {
        size_t start = first_posting->positions[i];
        size_t term_index;
        bool matched = true;
        for (term_index = 1U; term_index < phrase_count; ++term_index) {
            const TermEntry *entry = index_find_term(index, phrase_terms[term_index]);
            const Posting *posting = index_get_posting(entry, doc_id);
            size_t expected;
            if (start > SIZE_MAX - term_index) {
                matched = false;
                break;
            }
            expected = start + term_index;
            if (!position_exists(posting, expected)) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return true;
        }
    }
    return false;
}

static bool query_term_is_unique(char *const *unique_terms, size_t count, const char *candidate)
{
    size_t i;
    for (i = 0U; i < count; ++i) {
        if (strcmp(unique_terms[i], candidate) == 0) {
            return false;
        }
    }
    return true;
}

bool search_query(const SearchIndex *index,
                  const char *query,
                  SearchMode mode,
                  bool exact_phrase,
                  SearchResults *results,
                  char *error_buffer,
                  size_t error_buffer_size)
{
    char **query_tokens = NULL;
    char **unique_terms = NULL;
    size_t query_count = 0U;
    size_t unique_count = 0U;
    size_t doc_id;

    if (index == NULL || query == NULL || results == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Invalid search arguments.");
        return false;
    }
    if (mode != SEARCH_AND && mode != SEARCH_OR) {
        ms_set_error(error_buffer, error_buffer_size, "Unknown search mode.");
        return false;
    }
    results->items = NULL;
    results->count = 0U;

    if (!tokenizer_tokenize_string(query, index->stop_words_enabled, &query_tokens,
                                   &query_count, error_buffer, error_buffer_size)) {
        return false;
    }
    if (query_count == 0U) {
        tokenizer_free_tokens(query_tokens, query_count);
        ms_set_error(error_buffer, error_buffer_size, "Please enter at least one searchable word.");
        return false;
    }

    unique_terms = (char **)malloc(query_count * sizeof(*unique_terms));
    if (unique_terms == NULL) {
        tokenizer_free_tokens(query_tokens, query_count);
        ms_set_error(error_buffer, error_buffer_size, "Out of memory while preparing search query.");
        return false;
    }
    for (doc_id = 0U; doc_id < query_count; ++doc_id) {
        if (query_term_is_unique(unique_terms, unique_count, query_tokens[doc_id])) {
            unique_terms[unique_count++] = query_tokens[doc_id];
        }
    }

    if (index->document_count > SIZE_MAX / sizeof(*results->items)) {
        free(unique_terms);
        tokenizer_free_tokens(query_tokens, query_count);
        ms_set_error(error_buffer, error_buffer_size, "Too many indexed documents for search results.");
        return false;
    }
    results->items = (SearchResult *)malloc(index->document_count * sizeof(*results->items));
    if (index->document_count > 0U && results->items == NULL) {
        free(unique_terms);
        tokenizer_free_tokens(query_tokens, query_count);
        ms_set_error(error_buffer, error_buffer_size, "Out of memory while allocating search results.");
        return false;
    }

    for (doc_id = 0U; doc_id < index->document_count; ++doc_id) {
        size_t matched_terms = 0U;
        size_t term_index;
        bool include_document;

        for (term_index = 0U; term_index < unique_count; ++term_index) {
            if (index_get_posting(index_find_term(index, unique_terms[term_index]), doc_id) != NULL) {
                ++matched_terms;
            }
        }

        if (exact_phrase) {
            include_document = document_has_phrase(index, doc_id, query_tokens, query_count);
        } else if (mode == SEARCH_AND) {
            include_document = matched_terms == unique_count;
        } else if (mode == SEARCH_OR) {
            include_document = matched_terms > 0U;
        } else {
            free(results->items);
            results->items = NULL;
            free(unique_terms);
            tokenizer_free_tokens(query_tokens, query_count);
            ms_set_error(error_buffer, error_buffer_size, "Unknown search mode.");
            return false;
        }

        if (include_document) {
            SearchResult *result = &results->items[results->count++];
            result->doc_id = doc_id;
            result->matched_terms = matched_terms;
            result->score = ranking_score_document(index, doc_id,
                                      (const char *const *)unique_terms, unique_count);
        }
    }

    if (results->count > 1U) {
        qsort(results->items, results->count, sizeof(*results->items), compare_search_results);
    }

    free(unique_terms);
    tokenizer_free_tokens(query_tokens, query_count);
    return true;
}

void search_results_destroy(SearchResults *results)
{
    if (results == NULL) {
        return;
    }
    free(results->items);
    results->items = NULL;
    results->count = 0U;
}

bool search_result_has_term(const SearchIndex *index, size_t doc_id, const char *term)
{
    return index_get_posting(index_find_term(index, term), doc_id) != NULL;
}
