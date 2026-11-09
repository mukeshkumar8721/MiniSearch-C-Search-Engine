#ifndef MINISEARCH_INDEX_H
#define MINISEARCH_INDEX_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t id;
    char *name;
    char *path;
    size_t word_count;
} Document;

typedef struct {
    size_t doc_id;
    size_t frequency;
    size_t *positions;
    size_t positions_count;
    size_t positions_capacity;
} Posting;

typedef struct TermEntry {
    char *term;
    Posting *postings;
    size_t posting_count;
    size_t posting_capacity;
    struct TermEntry *next;
} TermEntry;

typedef struct {
    Document *documents;
    size_t document_count;
    size_t document_capacity;
    TermEntry **buckets;
    size_t bucket_count;
    size_t unique_terms;
    size_t total_tokens;
    bool stop_words_enabled;
} SearchIndex;

bool index_init(SearchIndex *index, char *error_buffer, size_t error_buffer_size);
void index_destroy(SearchIndex *index);

/* target must have been initialized with index_init. Builds into a temporary index and replaces target only after success. */
bool index_build_corpus(SearchIndex *target,
                        const char *directory,
                        bool remove_stop_words,
                        size_t *skipped_files,
                        char *error_buffer,
                        size_t error_buffer_size);

const TermEntry *index_find_term(const SearchIndex *index, const char *term);
const Posting *index_get_posting(const TermEntry *entry, size_t doc_id);
bool index_insert_loaded_term(SearchIndex *index,
                              char *owned_term,
                              Posting *owned_postings,
                              size_t posting_count);
size_t index_estimated_memory_bytes(const SearchIndex *index);

#endif
