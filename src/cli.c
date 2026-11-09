#include "cli.h"
#include "ranking.h"
#include "search.h"
#include "storage.h"
#include "timer.h"
#include "tokenizer.h"
#include "util.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *term;
    size_t frequency;
} TermSummary;

static bool prompt_line(const char *prompt, char **value)
{
    (void)printf("%s", prompt);
    (void)fflush(stdout);
    if (!ms_read_line(stdin, value)) {
        return false;
    }
    ms_trim_whitespace(*value);
    return true;
}

static bool prompt_yes_no(const char *prompt, bool *answer)
{
    char *line = NULL;
    bool valid = false;
    while (!valid) {
        if (!prompt_line(prompt, &line)) {
            return false;
        }
        if (line[0] == 'y' || line[0] == 'Y') {
            *answer = true;
            valid = true;
        } else if (line[0] == 'n' || line[0] == 'N') {
            *answer = false;
            valid = true;
        } else {
            (void)printf("Please enter y or n.\n");
        }
        free(line);
        line = NULL;
    }
    return true;
}

static bool read_menu_choice(int *choice)
{
    char *line = NULL;
    char *end = NULL;
    long value;

    if (!prompt_line("Select an option: ", &line)) {
        return false;
    }
    errno = 0;
    value = strtol(line, &end, 10);
    while (end != NULL && (*end == ' ' || *end == '\t')) {
        ++end;
    }
    if (errno != 0 || end == line || end == NULL || *end != '\0' || value < 0L || value > 10L) {
        (void)printf("Invalid choice. Enter a number from 0 to 10.\n");
        free(line);
        *choice = -1;
        return true;
    }
    *choice = (int)value;
    free(line);
    return true;
}

static void print_banner(void)
{
    (void)printf("\n=============================================\n");
    (void)printf("                  MINISEARCH                 \n");
    (void)printf("        LIGHTWEIGHT SEARCH ENGINE IN C       \n");
    (void)printf("=============================================\n");
}

static void print_menu(void)
{
    (void)printf("\n1. Index documents\n");
    (void)printf("2. Search documents\n");
    (void)printf("3. List indexed documents\n");
    (void)printf("4. View search statistics\n");
    (void)printf("5. View most frequent terms\n");
    (void)printf("6. Save index\n");
    (void)printf("7. Load index\n");
    (void)printf("8. Rebuild index\n");
    (void)printf("9. About MiniSearch\n");
    (void)printf("10. Run benchmark\n");
    (void)printf("0. Exit\n\n");
}

static void index_documents(SearchIndex *index, bool rebuilding)
{
    char *directory = NULL;
    bool remove_stop_words = false;
    char error[256] = {0};
    size_t skipped = 0U;
    double start;
    double elapsed;

    if (!prompt_line("Corpus directory (example: data/sample_documents): ", &directory)) {
        return;
    }
    if (directory[0] == '\0') {
        (void)printf("A directory path is required.\n");
        free(directory);
        return;
    }
    if (!prompt_yes_no("Remove common English stop words? (y/n): ", &remove_stop_words)) {
        free(directory);
        return;
    }

    start = timer_now_seconds();
    if (!index_build_corpus(index, directory, remove_stop_words, &skipped, error, sizeof(error))) {
        (void)printf("Indexing failed: %s\n", error);
    } else {
        elapsed = timer_now_seconds() - start;
        (void)printf("\n%s complete.\n", rebuilding ? "Index rebuild" : "Indexing");
        (void)printf("Documents indexed : %zu\n", index->document_count);
        (void)printf("Unique terms      : %zu\n", index->unique_terms);
        (void)printf("Indexed tokens    : %zu\n", index->total_tokens);
        (void)printf("Skipped files     : %zu\n", skipped);
        (void)printf("Elapsed time      : %.6f seconds\n", elapsed);
        (void)printf("Stop-word filter  : %s\n", index->stop_words_enabled ? "enabled" : "disabled");
    }
    free(directory);
}

static void print_matched_terms(const SearchIndex *index,
                                size_t doc_id,
                                char *const *query_terms,
                                size_t query_count)
{
    size_t i;
    bool first = true;
    for (i = 0U; i < query_count; ++i) {
        size_t previous;
        bool duplicate = false;
        if (!search_result_has_term(index, doc_id, query_terms[i])) {
            continue;
        }
        for (previous = 0U; previous < i; ++previous) {
            if (strcmp(query_terms[previous], query_terms[i]) == 0) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        (void)printf("%s%s", first ? "" : ", ", query_terms[i]);
        first = false;
    }
    if (first) {
        (void)printf("(phrase match)");
    }
}

static void search_documents(const SearchIndex *index)
{
    char *query = NULL;
    char *mode_line = NULL;
    char error[256] = {0};
    SearchMode mode = SEARCH_AND;
    bool exact_phrase = false;
    SearchResults results = {NULL, 0U};
    char **query_terms = NULL;
    size_t query_count = 0U;
    size_t i;
    double start;
    double elapsed;

    if (index->document_count == 0U) {
        (void)printf("No documents are indexed. Choose option 1 first.\n");
        return;
    }
    if (!prompt_line("Enter search query: ", &query)) {
        return;
    }
    if (query[0] == '\0') {
        (void)printf("Search query cannot be empty.\n");
        free(query);
        return;
    }
    (void)printf("Search mode: 1 = AND, 2 = OR, 3 = exact phrase\n");
    if (!prompt_line("Choose mode: ", &mode_line)) {
        free(query);
        return;
    }
    if (strcmp(mode_line, "3") == 0) {
        exact_phrase = true;
    } else if (strcmp(mode_line, "2") == 0) {
        mode = SEARCH_OR;
    } else if (strcmp(mode_line, "1") != 0) {
        (void)printf("Invalid search mode.\n");
        free(query);
        free(mode_line);
        return;
    }
    free(mode_line);

    start = timer_now_seconds();
    if (!search_query(index, query, mode, exact_phrase, &results, error, sizeof(error))) {
        (void)printf("Search error: %s\n", error);
        free(query);
        return;
    }
    elapsed = timer_now_seconds() - start;
    (void)printf("\nSearch results (%zu matching document%s):\n\n",
                 results.count, results.count == 1U ? "" : "s");

    if (results.count == 0U) {
        (void)printf("No matching documents were found. Try another query.\n");
    } else {
        if (!tokenizer_tokenize_string(query, index->stop_words_enabled, &query_terms,
                                       &query_count, error, sizeof(error))) {
            (void)printf("Could not format matched terms: %s\n", error);
        }
        for (i = 0U; i < results.count && i < 10U; ++i) {
            const Document *document = &index->documents[results.items[i].doc_id];
            (void)printf("%zu. %s\n", i + 1U, document->name);
            (void)printf("   Path: %s\n", document->path);
            (void)printf("   Matched terms: ");
            if (query_terms != NULL) {
                print_matched_terms(index, document->id, query_terms, query_count);
            } else {
                (void)printf("not available");
            }
            (void)printf("\n   Relevance score: %.6f\n\n", results.items[i].score);
        }
        if (results.count > 10U) {
            (void)printf("Showing 10 of %zu results.\n", results.count);
        }
    }
    (void)printf("Search time: %.6f seconds\n", elapsed);

    tokenizer_free_tokens(query_terms, query_count);
    search_results_destroy(&results);
    free(query);
}

static void list_documents(const SearchIndex *index)
{
    size_t i;
    if (index->document_count == 0U) {
        (void)printf("No documents are indexed.\n");
        return;
    }
    (void)printf("\n%-5s %-32s %12s  %s\n", "ID", "DOCUMENT", "WORD COUNT", "PATH");
    (void)printf("--------------------------------------------------------------------------------\n");
    for (i = 0U; i < index->document_count; ++i) {
        const Document *document = &index->documents[i];
        (void)printf("%-5zu %-32.32s %12zu  %s\n", document->id,
                     document->name, document->word_count, document->path);
    }
}

static void print_statistics(const SearchIndex *index)
{
    double average = index->document_count == 0U ? 0.0 :
        (double)index->total_tokens / (double)index->document_count;
    (void)printf("\nMiniSearch index statistics\n");
    (void)printf("Documents indexed : %zu\n", index->document_count);
    (void)printf("Unique terms      : %zu\n", index->unique_terms);
    (void)printf("Indexed tokens    : %zu\n", index->total_tokens);
    (void)printf("Average document length: %.2f tokens\n", average);
    (void)printf("Approx. index memory: %zu bytes\n", index_estimated_memory_bytes(index));
    (void)printf("Stop-word filter  : %s\n", index->stop_words_enabled ? "enabled" : "disabled");
}

static int compare_term_summary(const void *left, const void *right)
{
    const TermSummary *a = (const TermSummary *)left;
    const TermSummary *b = (const TermSummary *)right;
    if (a->frequency < b->frequency) {
        return 1;
    }
    if (a->frequency > b->frequency) {
        return -1;
    }
    return strcmp(a->term, b->term);
}

static void show_frequent_terms(const SearchIndex *index)
{
    TermSummary *terms;
    size_t count = 0U;
    size_t i;
    if (index->unique_terms == 0U) {
        (void)printf("There are no indexed terms yet.\n");
        return;
    }
    terms = (TermSummary *)calloc(index->unique_terms, sizeof(*terms));
    if (terms == NULL) {
        (void)printf("Unable to allocate memory for term statistics.\n");
        return;
    }
    for (i = 0U; i < index->bucket_count; ++i) {
        const TermEntry *entry = index->buckets[i];
        while (entry != NULL) {
            size_t j;
            size_t frequency = 0U;
            for (j = 0U; j < entry->posting_count; ++j) {
                if (entry->postings[j].frequency > SIZE_MAX - frequency) {
                    frequency = SIZE_MAX;
                    break;
                }
                frequency += entry->postings[j].frequency;
            }
            terms[count].term = entry->term;
            terms[count].frequency = frequency;
            ++count;
            entry = entry->next;
        }
    }
    qsort(terms, count, sizeof(*terms), compare_term_summary);
    (void)printf("\n%-5s %-24s %12s\n", "RANK", "TERM", "OCCURRENCES");
    (void)printf("----------------------------------------------\n");
    for (i = 0U; i < count && i < 10U; ++i) {
        (void)printf("%-5zu %-24s %12zu\n", i + 1U, terms[i].term, terms[i].frequency);
    }
    free(terms);
}

static void save_index(const SearchIndex *index)
{
    char *path = NULL;
    char error[256] = {0};
    if (!prompt_line("Index file path [minisearch.idx]: ", &path)) {
        return;
    }
    if (path[0] == '\0') {
        free(path);
        path = ms_strdup("minisearch.idx");
        if (path == NULL) {
            (void)printf("Out of memory.\n");
            return;
        }
    }
    if (storage_save(index, path, error, sizeof(error))) {
        (void)printf("Index saved to '%s'.\n", path);
    } else {
        (void)printf("Save failed: %s\n", error);
    }
    free(path);
}

static void load_index(SearchIndex *index)
{
    char *path = NULL;
    char error[256] = {0};
    if (!prompt_line("Index file path [minisearch.idx]: ", &path)) {
        return;
    }
    if (path[0] == '\0') {
        free(path);
        path = ms_strdup("minisearch.idx");
        if (path == NULL) {
            (void)printf("Out of memory.\n");
            return;
        }
    }
    if (storage_load(index, path, error, sizeof(error))) {
        (void)printf("Index loaded successfully.\n");
    } else {
        (void)printf("Load failed; the current in-memory index was kept. %s\n", error);
    }
    free(path);
}

static void run_benchmark(void)
{
    SearchIndex benchmark_index;
    char *directory = NULL;
    char *query = NULL;
    char error[256] = {0};
    size_t skipped = 0U;
    SearchResults results = {NULL, 0U};
    double start;
    double indexing_time;
    double search_time;

    if (!prompt_line("Corpus directory for benchmark: ", &directory)) {
        return;
    }
    if (directory[0] == '\0') {
        (void)printf("A directory path is required.\n");
        free(directory);
        return;
    }
    if (!prompt_line("Benchmark search query: ", &query)) {
        free(directory);
        return;
    }
    if (!index_init(&benchmark_index, error, sizeof(error))) {
        (void)printf("Benchmark setup failed: %s\n", error);
        free(directory);
        free(query);
        return;
    }

    start = timer_now_seconds();
    if (!index_build_corpus(&benchmark_index, directory, false, &skipped, error, sizeof(error))) {
        (void)printf("Benchmark indexing failed: %s\n", error);
        index_destroy(&benchmark_index);
        free(directory);
        free(query);
        return;
    }
    indexing_time = timer_now_seconds() - start;

    start = timer_now_seconds();
    if (!search_query(&benchmark_index, query, SEARCH_OR, false, &results, error, sizeof(error))) {
        (void)printf("Benchmark search failed: %s\n", error);
        search_results_destroy(&results);
        index_destroy(&benchmark_index);
        free(directory);
        free(query);
        return;
    }
    search_time = timer_now_seconds() - start;

    (void)printf("\nBenchmark summary\n");
    (void)printf("Timer backend       : %s\n", timer_backend_name());
    (void)printf("Documents indexed   : %zu\n", benchmark_index.document_count);
    (void)printf("Unique terms        : %zu\n", benchmark_index.unique_terms);
    (void)printf("Indexed tokens      : %zu\n", benchmark_index.total_tokens);
    (void)printf("Skipped files       : %zu\n", skipped);
    (void)printf("Indexing time       : %.6f seconds\n", indexing_time);
    (void)printf("Search time         : %.6f seconds\n", search_time);
    (void)printf("Matching documents  : %zu\n", results.count);
    (void)printf("Estimated index RAM : %zu bytes\n", index_estimated_memory_bytes(&benchmark_index));
    (void)printf("Dataset              : %s\n", directory);
    (void)printf("Note: record your hardware and compiler separately when publishing benchmark results.\n");

    search_results_destroy(&results);
    index_destroy(&benchmark_index);
    free(directory);
    free(query);
}

static void show_about(void)
{
    (void)printf("\nMiniSearch — A Lightweight Full-Text Search Engine in C\n");
    (void)printf("Search smarter. Understand deeper.\n\n");
    (void)printf("MiniSearch is an educational C11 application that reads local text files,\n");
    (void)printf("builds an inverted index, retrieves matching documents and ranks them\n");
    (void)printf("using a smoothed TF-IDF score. It does not use an external search API.\n");
    (void)printf("\nTokenization is ASCII alphanumeric; corpus scanning is non-recursive.\n");
}

int cli_run(SearchIndex *index)
{
    bool running = true;
    if (index == NULL) {
        return 1;
    }
    print_banner();
    while (running) {
        int choice;
        print_menu();
        if (!read_menu_choice(&choice)) {
            break;
        }
        switch (choice) {
            case 0:
                running = false;
                break;
            case 1:
                index_documents(index, false);
                break;
            case 2:
                search_documents(index);
                break;
            case 3:
                list_documents(index);
                break;
            case 4:
                print_statistics(index);
                break;
            case 5:
                show_frequent_terms(index);
                break;
            case 6:
                save_index(index);
                break;
            case 7:
                load_index(index);
                break;
            case 8:
                index_documents(index, true);
                break;
            case 9:
                show_about();
                break;
            case 10:
                run_benchmark();
                break;
            default:
                break;
        }
    }
    (void)printf("MiniSearch closed. Goodbye.\n");
    return 0;
}
