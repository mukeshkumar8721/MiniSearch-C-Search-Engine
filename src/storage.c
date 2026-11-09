#include "storage.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORAGE_VERSION 1U
#define MAX_STORED_DOCUMENTS 1000000ULL
#define MAX_STORED_TERMS 10000000ULL
#define MAX_STORED_STRING 1048576ULL
#define MAX_STORED_POSTINGS 1000000ULL
#define MAX_STORED_POSITIONS 10000000ULL

static const unsigned char STORAGE_MAGIC[8] = {'M', 'S', 'E', 'A', 'R', 'C', 'H', '1'};

static bool write_u32(FILE *file, uint32_t value)
{
    unsigned char bytes[4];
    bytes[0] = (unsigned char)(value & 0xFFU);
    bytes[1] = (unsigned char)((value >> 8U) & 0xFFU);
    bytes[2] = (unsigned char)((value >> 16U) & 0xFFU);
    bytes[3] = (unsigned char)((value >> 24U) & 0xFFU);
    return fwrite(bytes, 1U, sizeof(bytes), file) == sizeof(bytes);
}

static bool write_u64(FILE *file, uint64_t value)
{
    unsigned char bytes[8];
    size_t i;
    for (i = 0U; i < sizeof(bytes); ++i) {
        bytes[i] = (unsigned char)((value >> (i * 8U)) & 0xFFU);
    }
    return fwrite(bytes, 1U, sizeof(bytes), file) == sizeof(bytes);
}

static bool read_u32(FILE *file, uint32_t *value)
{
    unsigned char bytes[4];
    if (fread(bytes, 1U, sizeof(bytes), file) != sizeof(bytes)) {
        return false;
    }
    *value = (uint32_t)bytes[0] |
             ((uint32_t)bytes[1] << 8U) |
             ((uint32_t)bytes[2] << 16U) |
             ((uint32_t)bytes[3] << 24U);
    return true;
}

static bool read_u64(FILE *file, uint64_t *value)
{
    unsigned char bytes[8];
    size_t i;
    uint64_t result = 0U;
    if (fread(bytes, 1U, sizeof(bytes), file) != sizeof(bytes)) {
        return false;
    }
    for (i = 0U; i < sizeof(bytes); ++i) {
        result |= (uint64_t)bytes[i] << (i * 8U);
    }
    *value = result;
    return true;
}

static bool write_document(const Document *document, FILE *file)
{
    size_t path_length = strlen(document->path);
    size_t name_length = strlen(document->name);
    if (!write_u64(file, (uint64_t)document->id) ||
        !write_u64(file, (uint64_t)document->word_count) ||
        !write_u64(file, (uint64_t)path_length) ||
        !write_u64(file, (uint64_t)name_length)) {
        return false;
    }
    return fwrite(document->path, 1U, path_length, file) == path_length &&
           fwrite(document->name, 1U, name_length, file) == name_length;
}

static bool storage_write_contents(const SearchIndex *index, FILE *file)
{
    size_t i;
    uint32_t flags = index->stop_words_enabled ? 1U : 0U;

    if (fwrite(STORAGE_MAGIC, 1U, sizeof(STORAGE_MAGIC), file) != sizeof(STORAGE_MAGIC) ||
        !write_u32(file, STORAGE_VERSION) || !write_u32(file, flags) ||
        !write_u64(file, (uint64_t)index->document_count) ||
        !write_u64(file, (uint64_t)index->unique_terms) ||
        !write_u64(file, (uint64_t)index->total_tokens)) {
        return false;
    }

    for (i = 0U; i < index->document_count; ++i) {
        if (!write_document(&index->documents[i], file)) {
            return false;
        }
    }

    for (i = 0U; i < index->bucket_count; ++i) {
        const TermEntry *entry = index->buckets[i];
        while (entry != NULL) {
            size_t term_length = strlen(entry->term);
            size_t j;
            if (!write_u64(file, (uint64_t)term_length) ||
                !write_u64(file, (uint64_t)entry->posting_count) ||
                fwrite(entry->term, 1U, term_length, file) != term_length) {
                return false;
            }
            for (j = 0U; j < entry->posting_count; ++j) {
                const Posting *posting = &entry->postings[j];
                size_t k;
                if (!write_u64(file, (uint64_t)posting->doc_id) ||
                    !write_u64(file, (uint64_t)posting->frequency) ||
                    !write_u64(file, (uint64_t)posting->positions_count)) {
                    return false;
                }
                for (k = 0U; k < posting->positions_count; ++k) {
                    if (!write_u64(file, (uint64_t)posting->positions[k])) {
                        return false;
                    }
                }
            }
            entry = entry->next;
        }
    }
    return true;
}

bool storage_save(const SearchIndex *index,
                  const char *file_path,
                  char *error_buffer,
                  size_t error_buffer_size)
{
    size_t path_length;
    char *temporary_path;
    FILE *file;
    bool write_ok;
    int close_status;

    if (index == NULL || file_path == NULL || file_path[0] == '\0') {
        ms_set_error(error_buffer, error_buffer_size, "A valid index and output path are required.");
        return false;
    }
    path_length = strlen(file_path);
    if (path_length > SIZE_MAX - 5U) {
        ms_set_error(error_buffer, error_buffer_size, "Index file path is too long.");
        return false;
    }
    temporary_path = (char *)malloc(path_length + 5U);
    if (temporary_path == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Out of memory while preparing index save.");
        return false;
    }
    (void)snprintf(temporary_path, path_length + 5U, "%s.tmp", file_path);

    file = fopen(temporary_path, "wb");
    if (file == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Cannot create temporary index file '%s'.", temporary_path);
        free(temporary_path);
        return false;
    }
    write_ok = storage_write_contents(index, file);
    close_status = fclose(file);
    if (!write_ok || close_status != 0) {
        (void)remove(temporary_path);
        ms_set_error(error_buffer, error_buffer_size, "Writing the index file failed.");
        free(temporary_path);
        return false;
    }

    if (rename(temporary_path, file_path) != 0) {
#ifdef _WIN32
        (void)remove(file_path);
        if (rename(temporary_path, file_path) == 0) {
            free(temporary_path);
            return true;
        }
#endif
        (void)remove(temporary_path);
        ms_set_error(error_buffer, error_buffer_size, "Could not move the saved index into place.");
        free(temporary_path);
        return false;
    }
    free(temporary_path);
    return true;
}

static bool u64_to_size(uint64_t value, size_t *converted)
{
    if (value > (uint64_t)SIZE_MAX) {
        return false;
    }
    *converted = (size_t)value;
    return true;
}

static char *read_string(FILE *file, uint64_t length_value, uint64_t maximum_length)
{
    size_t length;
    char *text;
    if (length_value == 0U || length_value > maximum_length ||
        !u64_to_size(length_value, &length) || length == SIZE_MAX) {
        return NULL;
    }
    text = (char *)malloc(length + 1U);
    if (text == NULL) {
        return NULL;
    }
    if (fread(text, 1U, length, file) != length || memchr(text, '\0', length) != NULL) {
        free(text);
        return NULL;
    }
    text[length] = '\0';
    return text;
}

static bool valid_term_text(const char *term)
{
    const unsigned char *cursor = (const unsigned char *)term;
    if (*cursor == 0U) {
        return false;
    }
    while (*cursor != 0U) {
        if (!((*cursor >= 'a' && *cursor <= 'z') ||
              (*cursor >= '0' && *cursor <= '9'))) {
            return false;
        }
        ++cursor;
    }
    return true;
}

static void free_postings(Posting *postings, size_t count)
{
    size_t i;
    if (postings == NULL) {
        return;
    }
    for (i = 0U; i < count; ++i) {
        free(postings[i].positions);
    }
    free(postings);
}

static bool load_document(SearchIndex *index, FILE *file, size_t expected_id)
{
    uint64_t id_value;
    uint64_t word_count_value;
    uint64_t path_length;
    uint64_t name_length;
    Document *grown;
    Document document;

    if (!read_u64(file, &id_value) || !read_u64(file, &word_count_value) ||
        !read_u64(file, &path_length) || !read_u64(file, &name_length) ||
        id_value != (uint64_t)expected_id || word_count_value > (uint64_t)SIZE_MAX) {
        return false;
    }
    memset(&document, 0, sizeof(document));
    document.path = read_string(file, path_length, MAX_STORED_STRING);
    document.name = read_string(file, name_length, MAX_STORED_STRING);
    if (document.path == NULL || document.name == NULL) {
        free(document.path);
        free(document.name);
        return false;
    }
    document.id = expected_id;
    document.word_count = (size_t)word_count_value;

    if (index->document_count == index->document_capacity) {
        size_t new_capacity = index->document_capacity == 0U ? 8U : index->document_capacity * 2U;
        if (new_capacity < index->document_capacity || new_capacity > SIZE_MAX / sizeof(*grown)) {
            free(document.path);
            free(document.name);
            return false;
        }
        grown = (Document *)realloc(index->documents, new_capacity * sizeof(*grown));
        if (grown == NULL) {
            free(document.path);
            free(document.name);
            return false;
        }
        index->documents = grown;
        index->document_capacity = new_capacity;
    }
    index->documents[index->document_count++] = document;
    if (document.word_count > SIZE_MAX - index->total_tokens) {
        return false;
    }
    index->total_tokens += document.word_count;
    return true;
}

static bool load_term(SearchIndex *index,
                      FILE *file,
                      size_t document_count,
                      uint64_t *indexed_occurrences,
                      size_t *document_occurrences)
{
    uint64_t term_length_value;
    uint64_t posting_count_value;
    size_t posting_count;
    size_t i;
    char *term = NULL;
    Posting *postings = NULL;

    if (!read_u64(file, &term_length_value) || !read_u64(file, &posting_count_value) ||
        posting_count_value == 0U || posting_count_value > MAX_STORED_POSTINGS ||
        !u64_to_size(posting_count_value, &posting_count)) {
        return false;
    }
    term = read_string(file, term_length_value, 256U);
    if (term == NULL || !valid_term_text(term) || index_find_term(index, term) != NULL) {
        free(term);
        return false;
    }

    postings = (Posting *)calloc(posting_count, sizeof(*postings));
    if (postings == NULL) {
        free(term);
        return false;
    }

    for (i = 0U; i < posting_count; ++i) {
        uint64_t doc_id_value;
        uint64_t frequency_value;
        uint64_t position_count_value;
        size_t doc_id;
        size_t position_count;
        size_t j;
        size_t previous_position = 0U;

        if (!read_u64(file, &doc_id_value) || !read_u64(file, &frequency_value) ||
            !read_u64(file, &position_count_value) ||
            !u64_to_size(doc_id_value, &doc_id) || doc_id >= document_count ||
            frequency_value == 0U || frequency_value != position_count_value ||
            position_count_value > MAX_STORED_POSITIONS ||
            !u64_to_size(position_count_value, &position_count) ||
            (i > 0U && doc_id <= postings[i - 1U].doc_id) ||
            position_count > index->documents[doc_id].word_count) {
            free(term);
            free_postings(postings, posting_count);
            return false;
        }
        postings[i].doc_id = doc_id;
        postings[i].frequency = (size_t)frequency_value;
        postings[i].positions = (size_t *)malloc(position_count * sizeof(*postings[i].positions));
        if (postings[i].positions == NULL) {
            free(term);
            free_postings(postings, posting_count);
            return false;
        }
        postings[i].positions_count = position_count;
        postings[i].positions_capacity = position_count;
        for (j = 0U; j < position_count; ++j) {
            uint64_t position_value;
            size_t position;
            if (!read_u64(file, &position_value) || !u64_to_size(position_value, &position) ||
                position >= index->documents[doc_id].word_count ||
                (j > 0U && position <= previous_position) ||
                *indexed_occurrences == UINT64_MAX ||
                document_occurrences[doc_id] == SIZE_MAX) {
                free(term);
                free_postings(postings, posting_count);
                return false;
            }
            postings[i].positions[j] = position;
            previous_position = position;
            ++(*indexed_occurrences);
            ++document_occurrences[doc_id];
        }
    }

    if (!index_insert_loaded_term(index, term, postings, posting_count)) {
        free(term);
        free_postings(postings, posting_count);
        return false;
    }
    return true;
}

bool storage_load(SearchIndex *target,
                  const char *file_path,
                  char *error_buffer,
                  size_t error_buffer_size)
{
    FILE *file;
    SearchIndex temporary;
    unsigned char magic[8];
    uint32_t version;
    uint32_t flags;
    uint64_t doc_count_value;
    uint64_t term_count_value;
    uint64_t total_tokens_value;
    uint64_t indexed_occurrences = 0U;
    size_t *document_occurrences = NULL;
    size_t document_count;
    size_t term_count;
    size_t i;
    int trailing;

    if (target == NULL || file_path == NULL || file_path[0] == '\0') {
        ms_set_error(error_buffer, error_buffer_size, "A valid target index and input path are required.");
        return false;
    }
    file = fopen(file_path, "rb");
    if (file == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Cannot open index file '%s'.", file_path);
        return false;
    }
    if (!index_init(&temporary, error_buffer, error_buffer_size)) {
        (void)fclose(file);
        return false;
    }

    if (fread(magic, 1U, sizeof(magic), file) != sizeof(magic) ||
        memcmp(magic, STORAGE_MAGIC, sizeof(magic)) != 0 ||
        !read_u32(file, &version) || version != STORAGE_VERSION ||
        !read_u32(file, &flags) || flags > 1U ||
        !read_u64(file, &doc_count_value) || !read_u64(file, &term_count_value) ||
        !read_u64(file, &total_tokens_value) ||
        doc_count_value > MAX_STORED_DOCUMENTS || term_count_value > MAX_STORED_TERMS ||
        !u64_to_size(doc_count_value, &document_count) ||
        !u64_to_size(term_count_value, &term_count)) {
        ms_set_error(error_buffer, error_buffer_size, "Invalid, corrupted or unsupported MiniSearch index header.");
        goto failure;
    }
    temporary.stop_words_enabled = flags == 1U;

    for (i = 0U; i < document_count; ++i) {
        if (!load_document(&temporary, file, i)) {
            ms_set_error(error_buffer, error_buffer_size, "Invalid document record in index file.");
            goto failure;
        }
    }
    if (document_count > 0U) {
        document_occurrences = (size_t *)calloc(document_count, sizeof(*document_occurrences));
        if (document_occurrences == NULL) {
            ms_set_error(error_buffer, error_buffer_size, "Out of memory while validating index postings.");
            goto failure;
        }
    }
    for (i = 0U; i < term_count; ++i) {
        if (!load_term(&temporary, file, document_count, &indexed_occurrences, document_occurrences)) {
            ms_set_error(error_buffer, error_buffer_size, "Invalid or corrupted term/posting data in index file.");
            goto failure;
        }
    }

    trailing = fgetc(file);
    if (trailing != EOF || ferror(file) != 0 ||
        total_tokens_value != (uint64_t)temporary.total_tokens ||
        indexed_occurrences != total_tokens_value) {
        ms_set_error(error_buffer, error_buffer_size, "Index file is incomplete or contains inconsistent data.");
        goto failure;
    }
    for (i = 0U; i < document_count; ++i) {
        if (document_occurrences[i] != temporary.documents[i].word_count) {
            ms_set_error(error_buffer, error_buffer_size, "Posting frequencies do not match a document's token count.");
            goto failure;
        }
    }
    free(document_occurrences);
    (void)fclose(file);
    index_destroy(target);
    *target = temporary;
    return true;

failure:
    free(document_occurrences);
    (void)fclose(file);
    index_destroy(&temporary);
    return false;
}
