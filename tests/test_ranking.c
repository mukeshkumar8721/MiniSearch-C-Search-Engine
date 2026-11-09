#include "index.h"
#include "ranking.h"
#include "test_common.h"

#include <math.h>

int main(void)
{
    const char *directory = "tests/tmp_ranking_corpus";
    char *a_path = test_join_path(directory, "a.txt");
    char *b_path = test_join_path(directory, "b.txt");
    SearchIndex index;
    char error[256] = {0};
    double expected;
    double actual;
    const char *terms[] = {"apple", "banana"};

    TEST_ASSERT(a_path != NULL && b_path != NULL);
    TEST_ASSERT(test_make_directory(directory));
    TEST_ASSERT(test_write_file(a_path, "apple apple banana"));
    TEST_ASSERT(test_write_file(b_path, "apple carrot"));
    TEST_ASSERT(index_init(&index, error, sizeof(error)));
    TEST_ASSERT(index_build_corpus(&index, directory, false, NULL, error, sizeof(error)));

    TEST_ASSERT(fabs(ranking_idf(&index, 2U) - 1.0) < 1e-10);
    TEST_ASSERT(fabs(ranking_idf(&index, 1U) - (log(3.0 / 2.0) + 1.0)) < 1e-10);
    expected = (2.0 / 3.0) * 1.0 + (1.0 / 3.0) * (log(3.0 / 2.0) + 1.0);
    actual = ranking_score_document(&index, 0U, terms, 2U);
    TEST_ASSERT(fabs(actual - expected) < 1e-10);
    TEST_ASSERT(ranking_term_score(&index, 1U, "carrot") > 0.0);

    index_destroy(&index);
    test_remove_file(a_path);
    test_remove_file(b_path);
    free(a_path);
    free(b_path);
    (void)remove(directory);
    (void)printf("PASS: ranking tests\n");
    return 0;
}
