#include "index.h"
#include "search.h"
#include "storage.h"
#include "test_common.h"

int main(void)
{
    const char *directory = "tests/tmp_storage_corpus";
    const char *index_path = "tests/tmp_minisearch.idx";
    const char *broken_path = "tests/tmp_broken.idx";
    char *a_path = test_join_path(directory, "a.txt");
    char *b_path = test_join_path(directory, "b.txt");
    SearchIndex original;
    SearchIndex loaded;
    SearchResults results = {NULL, 0U};
    char error[256] = {0};
    FILE *broken;
    FILE *saved;
    long saved_length;
    unsigned char *saved_bytes;

    TEST_ASSERT(a_path != NULL && b_path != NULL);
    TEST_ASSERT(test_make_directory(directory));
    TEST_ASSERT(test_write_file(a_path, "C search engine search"));
    TEST_ASSERT(test_write_file(b_path, "A small search index"));
    TEST_ASSERT(index_init(&original, error, sizeof(error)));
    TEST_ASSERT(index_build_corpus(&original, directory, true, NULL, error, sizeof(error)));
    TEST_ASSERT(storage_save(&original, index_path, error, sizeof(error)));
    TEST_ASSERT(index_init(&loaded, error, sizeof(error)));
    TEST_ASSERT(storage_load(&loaded, index_path, error, sizeof(error)));
    TEST_ASSERT(loaded.document_count == original.document_count);
    TEST_ASSERT(loaded.unique_terms == original.unique_terms);
    TEST_ASSERT(loaded.total_tokens == original.total_tokens);
    TEST_ASSERT(loaded.stop_words_enabled == original.stop_words_enabled);
    TEST_ASSERT(search_query(&loaded, "search engine", SEARCH_AND, true,
                             &results, error, sizeof(error)));
    TEST_ASSERT(results.count == 1U);
    search_results_destroy(&results);

    saved = fopen(index_path, "rb");
    TEST_ASSERT(saved != NULL);
    TEST_ASSERT(fseek(saved, 0L, SEEK_END) == 0);
    saved_length = ftell(saved);
    TEST_ASSERT(saved_length > 1L);
    TEST_ASSERT(fseek(saved, 0L, SEEK_SET) == 0);
    saved_bytes = (unsigned char *)malloc((size_t)saved_length);
    TEST_ASSERT(saved_bytes != NULL);
    TEST_ASSERT(fread(saved_bytes, 1U, (size_t)saved_length, saved) == (size_t)saved_length);
    TEST_ASSERT(fclose(saved) == 0);
    broken = fopen(broken_path, "wb");
    TEST_ASSERT(broken != NULL);
    TEST_ASSERT(fwrite(saved_bytes, 1U, (size_t)saved_length - 1U, broken) == (size_t)saved_length - 1U);
    TEST_ASSERT(fclose(broken) == 0);
    free(saved_bytes);
    TEST_ASSERT(!storage_load(&loaded, broken_path, error, sizeof(error)));
    TEST_ASSERT(loaded.document_count == original.document_count);
    TEST_ASSERT(loaded.unique_terms == original.unique_terms);

    index_destroy(&original);
    index_destroy(&loaded);
    test_remove_file(a_path);
    test_remove_file(b_path);
    test_remove_file(index_path);
    test_remove_file("tests/tmp_minisearch.idx.tmp");
    test_remove_file(broken_path);
    free(a_path);
    free(b_path);
    (void)remove(directory);
    (void)printf("PASS: storage tests\n");
    return 0;
}
