#include "index.h"
#include "document.h"
#include "tokenizer.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUCKET_COUNT 257U

typedef enum {
    ADD_FILE_OK = 0,
    ADD_FILE_SKIPPED = 1,
    ADD_FILE_ERROR = -1
} AddFileStatus;

static size_t hash_term(const char *term)
{
    const unsigned char *cursor = (const unsigned char *)term;
    size_t hash = (size_t)14695981039346656037ULL;
    while (*cursor != 0U) {
        hash ^= (size_t)*cursor;
        hash *= (size_t)1099511628211ULL;
        ++cursor;
    }
    return hash;
}

static bool resize_buckets(SearchIndex *index, size_t new_count)
{
    TermEntry **new_buckets;
    size_t i;

    if (new_count < index->bucket_count || new_count > SIZE_MAX / sizeof(*new_buckets)) {
        return false;
    }
    new_buckets = (TermEntry **)calloc(new_count, sizeof(*new_buckets));
    if (new_buckets == NULL) {
        return false;
    }

    for (i = 0U; i < index->bucket_count; ++i) {
        TermEntry *entry = index->buckets[i];
        while (entry != NULL) {
            TermEntry *next = entry->next;
            size_t bucket = hash_term(entry->term) % new_count;
            entry->next = new_buckets[bucket];
            new_buckets[bucket] = entry;
            entry = next;
        }
    }

    free(index->buckets);
    index->buckets = new_buckets;
    index->bucket_count = new_count;
    return true;
}

bool index_init(SearchIndex *index, char *error_buffer, size_t error_buffer_size)
{
    if (index == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Index pointer is null.");
        return false;
    }
    memset(index, 0, sizeof(*index));
    index->buckets = (TermEntry **)calloc(INITIAL_BUCKET_COUNT, sizeof(*index->buckets));
    if (index->buckets == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Unable to allocate the index hash table.");
        return false;
    }
    index->bucket_count = INITIAL_BUCKET_COUNT;
    return true;
}

void index_destroy(SearchIndex *index)
{
    size_t i;

    if (index == NULL) {
        return;
    }
    for (i = 0U; i < index->document_count; ++i) {
        free(index->documents[i].name);
        free(index->documents[i].path);
    }
    free(index->documents);

    for (i = 0U; i < index->bucket_count; ++i) {
        TermEntry *entry = index->buckets == NULL ? NULL : index->buckets[i];
        while (entry != NULL) {
            TermEntry *next_entry = entry->next;
            size_t j;
            for (j = 0U; j < entry->posting_count; ++j) {
                free(entry->postings[j].positions);
            }
            free(entry->postings);
            free(entry->term);
            free(entry);
            entry = next_entry;
        }
    }
    free(index->buckets);
    memset(index, 0, sizeof(*index));
}

const TermEntry *index_find_term(const SearchIndex *index, const char *term)
{
    size_t bucket;
    const TermEntry *entry;

    if (index == NULL || index->buckets == NULL || index->bucket_count == 0U || term == NULL) {
        return NULL;
    }
    bucket = hash_term(term) % index->bucket_count;
    entry = index->buckets[bucket];
    while (entry != NULL) {
        if (strcmp(entry->term, term) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

const Posting *index_get_posting(const TermEntry *entry, size_t doc_id)
{
    size_t low = 0U;
    size_t high;

    if (entry == NULL) {
        return NULL;
    }
    high = entry->posting_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2U;
        if (entry->postings[middle].doc_id == doc_id) {
            return &entry->postings[middle];
        }
        if (entry->postings[middle].doc_id < doc_id) {
            low = middle + 1U;
        } else {
            high = middle;
        }
    }
    return NULL;
}

static bool append_document(SearchIndex *index, const char *path)
{
    Document document;
    Document *grown;

    if (index->document_count == index->document_capacity) {
        size_t new_capacity = index->document_capacity == 0U ? 8U : index->document_capacity * 2U;
        if (new_capacity < index->document_capacity || new_capacity > SIZE_MAX / sizeof(*grown)) {
            return false;
        }
        grown = (Document *)realloc(index->documents, new_capacity * sizeof(*grown));
        if (grown == NULL) {
            return false;
        }
        index->documents = grown;
        index->document_capacity = new_capacity;
    }

    document.id = index->document_count;
    document.name = document_basename_copy(path);
    document.path = ms_strdup(path);
    document.word_count = 0U;
    if (document.name == NULL || document.path == NULL) {
        free(document.name);
        free(document.path);
        return false;
    }
    index->documents[index->document_count++] = document;
    return true;
}

static bool posting_append_position(Posting *posting, size_t position)
{
    if (posting->positions_count == posting->positions_capacity) {
        size_t new_capacity = posting->positions_capacity == 0U ? 4U : posting->positions_capacity * 2U;
        size_t *grown;
        if (new_capacity < posting->positions_capacity || new_capacity > SIZE_MAX / sizeof(*grown)) {
            return false;
        }
        grown = (size_t *)realloc(posting->positions, new_capacity * sizeof(*grown));
        if (grown == NULL) {
            return false;
        }
        posting->positions = grown;
        posting->positions_capacity = new_capacity;
    }
    posting->positions[posting->positions_count++] = position;
    posting->frequency = posting->positions_count;
    return true;
}

static TermEntry *create_term(SearchIndex *index, const char *term)
{
    TermEntry *entry;
    size_t bucket;

    if (index->unique_terms >= index->bucket_count * 3U / 4U) {
        size_t new_count = index->bucket_count * 2U + 1U;
        if (new_count <= index->bucket_count || !resize_buckets(index, new_count)) {
            return NULL;
        }
    }

    entry = (TermEntry *)calloc(1U, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->term = ms_strdup(term);
    if (entry->term == NULL) {
        free(entry);
        return NULL;
    }
    bucket = hash_term(term) % index->bucket_count;
    entry->next = index->buckets[bucket];
    index->buckets[bucket] = entry;
    ++index->unique_terms;
    return entry;
}

static bool add_occurrence(SearchIndex *index, const char *term, size_t doc_id, size_t position)
{
    const TermEntry *found = index_find_term(index, term);
    TermEntry *entry = (TermEntry *)found;
    Posting *posting;

    if (entry == NULL) {
        entry = create_term(index, term);
        if (entry == NULL) {
            return false;
        }
    }

    if (entry->posting_count > 0U &&
        entry->postings[entry->posting_count - 1U].doc_id == doc_id) {
        posting = &entry->postings[entry->posting_count - 1U];
    } else {
        if (entry->posting_count > 0U &&
            entry->postings[entry->posting_count - 1U].doc_id >= doc_id) {
            return false;
        }
        if (entry->posting_count == entry->posting_capacity) {
            size_t new_capacity = entry->posting_capacity == 0U ? 4U : entry->posting_capacity * 2U;
            Posting *grown;
            if (new_capacity < entry->posting_capacity || new_capacity > SIZE_MAX / sizeof(*grown)) {
                return false;
            }
            grown = (Posting *)realloc(entry->postings, new_capacity * sizeof(*grown));
            if (grown == NULL) {
                return false;
            }
            entry->postings = grown;
            entry->posting_capacity = new_capacity;
        }
        posting = &entry->postings[entry->posting_count++];
        memset(posting, 0, sizeof(*posting));
        posting->doc_id = doc_id;
    }
    return posting_append_position(posting, position);
}

static AddFileStatus add_file(SearchIndex *index,
                              const char *path,
                              char *error_buffer,
                              size_t error_buffer_size)
{
    FILE *file = fopen(path, "rb");
    size_t doc_id;

    if (file == NULL) {
        return ADD_FILE_SKIPPED;
    }
    if (!append_document(index, path)) {
        (void)fclose(file);
        ms_set_error(error_buffer, error_buffer_size, "Out of memory while adding document '%s'.", path);
        return ADD_FILE_ERROR;
    }
    doc_id = index->document_count - 1U;

    for (;;) {
        char *token = NULL;
        TokenizerStatus status = tokenizer_next(file, &token);
        if (status == TOKENIZER_EOF) {
            break;
        }
        if (status == TOKENIZER_ERROR) {
            (void)fclose(file);
            ms_set_error(error_buffer, error_buffer_size, "Could not read or tokenize '%s'.", path);
            return ADD_FILE_ERROR;
        }

        if (index->stop_words_enabled && tokenizer_is_stop_word(token)) {
            free(token);
            continue;
        }
        if (index->documents[doc_id].word_count == SIZE_MAX || index->total_tokens == SIZE_MAX ||
            !add_occurrence(index, token, doc_id, index->documents[doc_id].word_count)) {
            free(token);
            (void)fclose(file);
            ms_set_error(error_buffer, error_buffer_size, "Unable to add a token from '%s' (memory or size limit).", path);
            return ADD_FILE_ERROR;
        }
        ++index->documents[doc_id].word_count;
        ++index->total_tokens;
        free(token);
    }

    if (fclose(file) != 0) {
        ms_set_error(error_buffer, error_buffer_size, "An error occurred while closing '%s'.", path);
        return ADD_FILE_ERROR;
    }
    return ADD_FILE_OK;
}

bool index_build_corpus(SearchIndex *target,
                        const char *directory,
                        bool remove_stop_words,
                        size_t *skipped_files,
                        char *error_buffer,
                        size_t error_buffer_size)
{
    SearchIndex temporary;
    DocumentPathList paths;
    size_t skipped = 0U;
    size_t i;

    if (target == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Target index pointer is null.");
        return false;
    }
    if (!index_init(&temporary, error_buffer, error_buffer_size)) {
        return false;
    }
    temporary.stop_words_enabled = remove_stop_words;

    if (!document_collect_txt_files(directory, &paths, error_buffer, error_buffer_size)) {
        index_destroy(&temporary);
        return false;
    }

    for (i = 0U; i < paths.count; ++i) {
        AddFileStatus status = add_file(&temporary, paths.items[i], error_buffer, error_buffer_size);
        if (status == ADD_FILE_ERROR) {
            document_path_list_destroy(&paths);
            index_destroy(&temporary);
            return false;
        }
        if (status == ADD_FILE_SKIPPED) {
            ++skipped;
        }
    }

    document_path_list_destroy(&paths);
    index_destroy(target);
    *target = temporary;
    if (skipped_files != NULL) {
        *skipped_files = skipped;
    }
    return true;
}

bool index_insert_loaded_term(SearchIndex *index,
                              char *owned_term,
                              Posting *owned_postings,
                              size_t posting_count)
{
    TermEntry *entry;
    size_t bucket;

    if (index == NULL || owned_term == NULL || owned_postings == NULL ||
        index_find_term(index, owned_term) != NULL) {
        return false;
    }
    if (index->unique_terms >= index->bucket_count * 3U / 4U) {
        size_t new_count = index->bucket_count * 2U + 1U;
        if (new_count <= index->bucket_count || !resize_buckets(index, new_count)) {
            return false;
        }
    }
    entry = (TermEntry *)calloc(1U, sizeof(*entry));
    if (entry == NULL) {
        return false;
    }
    entry->term = owned_term;
    entry->postings = owned_postings;
    entry->posting_count = posting_count;
    entry->posting_capacity = posting_count;
    bucket = hash_term(owned_term) % index->bucket_count;
    entry->next = index->buckets[bucket];
    index->buckets[bucket] = entry;
    ++index->unique_terms;
    return true;
}

size_t index_estimated_memory_bytes(const SearchIndex *index)
{
    size_t total = 0U;
    size_t i;

    if (index == NULL) {
        return 0U;
    }
    total += sizeof(*index);
    total += index->bucket_count * sizeof(*index->buckets);
    total += index->document_capacity * sizeof(*index->documents);
    for (i = 0U; i < index->document_count; ++i) {
        total += strlen(index->documents[i].name) + 1U;
        total += strlen(index->documents[i].path) + 1U;
    }
    for (i = 0U; i < index->bucket_count; ++i) {
        const TermEntry *entry = index->buckets[i];
        while (entry != NULL) {
            size_t j;
            total += sizeof(*entry) + strlen(entry->term) + 1U;
            total += entry->posting_capacity * sizeof(*entry->postings);
            for (j = 0U; j < entry->posting_count; ++j) {
                total += entry->postings[j].positions_capacity * sizeof(*entry->postings[j].positions);
            }
            entry = entry->next;
        }
    }
    return total;
}
