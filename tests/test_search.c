#include "index.h"
#include "search.h"
#include "test_common.h"

int main(void)
{
    const char *directory = "tests/tmp_search_corpus";
    char *a_path = test_join_path(directory, "a.txt");
    char *b_path = test_join_path(directory, "b.txt");
    char *c_path = test_join_path(directory, "c.txt");
    SearchIndex index;
    SearchResults results = {NULL, 0U};
    char error[256] = {0};

    TEST_ASSERT(a_path != NULL && b_path != NULL && c_path != NULL);
    TEST_ASSERT(test_make_directory(directory));
    TEST_ASSERT(test_write_file(a_path, "Data structures are useful"));
    TEST_ASSERT(test_write_file(b_path, "Data algorithms"));
    TEST_ASSERT(test_write_file(c_path, "Structures and algorithms"));
    TEST_ASSERT(index_init(&index, error, sizeof(error)));
    TEST_ASSERT(index_build_corpus(&index, directory, false, NULL, error, sizeof(error)));

    TEST_ASSERT(search_query(&index, "DATA structures", SEARCH_AND, false,
                             &results, error, sizeof(error)));
    TEST_ASSERT(results.count == 1U);
    TEST_ASSERT(strcmp(index.documents[results.items[0].doc_id].name, "a.txt") == 0);
    search_results_destroy(&results);

    TEST_ASSERT(search_query(&index, "data structures", SEARCH_OR, false,
                             &results, error, sizeof(error)));
    TEST_ASSERT(results.count == 3U);
    search_results_destroy(&results);

    TEST_ASSERT(search_query(&index, "data structures", SEARCH_AND, true,
                             &results, error, sizeof(error)));
    TEST_ASSERT(results.count == 1U);
    TEST_ASSERT(strcmp(index.documents[results.items[0].doc_id].name, "a.txt") == 0);
    search_results_destroy(&results);

    TEST_ASSERT(search_query(&index, "missingword", SEARCH_OR, false,
                             &results, error, sizeof(error)));
    TEST_ASSERT(results.count == 0U);
    search_results_destroy(&results);

    TEST_ASSERT(!search_query(&index, "   !!! ", SEARCH_OR, false,
                              &results, error, sizeof(error)));

    index_destroy(&index);
    test_remove_file(a_path);
    test_remove_file(b_path);
    test_remove_file(c_path);
    free(a_path);
    free(b_path);
    free(c_path);
    (void)remove(directory);
    (void)printf("PASS: search tests\n");
    return 0;
}
