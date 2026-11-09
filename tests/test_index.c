#include "index.h"
#include "test_common.h"

int main(void)
{
    const char *directory = "tests/tmp_index_corpus";
    char *a_path = test_join_path(directory, "a.txt");
    char *b_path = test_join_path(directory, "b.txt");
    char *empty_path = test_join_path(directory, "empty.txt");
    SearchIndex index;
    char error[256] = {0};
    size_t skipped = 0U;
    const TermEntry *systems;

    TEST_ASSERT(a_path != NULL && b_path != NULL && empty_path != NULL);
    TEST_ASSERT(test_make_directory(directory));
    TEST_ASSERT(test_write_file(a_path, "C systems systems"));
    TEST_ASSERT(test_write_file(b_path, "Search engine"));
    TEST_ASSERT(test_write_file(empty_path, ""));
    TEST_ASSERT(index_init(&index, error, sizeof(error)));
    TEST_ASSERT(index_build_corpus(&index, directory, false, &skipped, error, sizeof(error)));
    TEST_ASSERT(skipped == 0U);
    TEST_ASSERT(index.document_count == 3U);
    TEST_ASSERT(index.total_tokens == 5U);
    TEST_ASSERT(index.unique_terms == 4U);
    systems = index_find_term(&index, "systems");
    TEST_ASSERT(systems != NULL);
    TEST_ASSERT(systems->posting_count == 1U);
    TEST_ASSERT(systems->postings[0].frequency == 2U);
    TEST_ASSERT(systems->postings[0].positions_count == 2U);

    index_destroy(&index);
    test_remove_file(a_path);
    test_remove_file(b_path);
    test_remove_file(empty_path);
    free(a_path);
    free(b_path);
    free(empty_path);
    (void)remove(directory);
    (void)printf("PASS: index tests\n");
    return 0;
}
