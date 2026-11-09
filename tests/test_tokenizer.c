#include "tokenizer.h"
#include "test_common.h"

int main(void)
{
    FILE *stream = tmpfile();
    char *token = NULL;
    char **query_tokens = NULL;
    size_t query_count = 0U;
    char error[128] = {0};

    TEST_ASSERT(stream != NULL);
    TEST_ASSERT(fputs("Hello, WORLD42! C11\nsearch-engine", stream) >= 0);
    rewind(stream);

    TEST_ASSERT(tokenizer_next(stream, &token) == TOKENIZER_TOKEN);
    TEST_ASSERT(strcmp(token, "hello") == 0);
    free(token);
    token = NULL;
    TEST_ASSERT(tokenizer_next(stream, &token) == TOKENIZER_TOKEN);
    TEST_ASSERT(strcmp(token, "world42") == 0);
    free(token);
    token = NULL;
    TEST_ASSERT(tokenizer_next(stream, &token) == TOKENIZER_TOKEN);
    TEST_ASSERT(strcmp(token, "c11") == 0);
    free(token);
    token = NULL;
    TEST_ASSERT(tokenizer_next(stream, &token) == TOKENIZER_TOKEN);
    TEST_ASSERT(strcmp(token, "search") == 0);
    free(token);
    token = NULL;
    TEST_ASSERT(tokenizer_next(stream, &token) == TOKENIZER_TOKEN);
    TEST_ASSERT(strcmp(token, "engine") == 0);
    free(token);
    TEST_ASSERT(tokenizer_next(stream, &token) == TOKENIZER_EOF);
    (void)fclose(stream);

    TEST_ASSERT(tokenizer_tokenize_string("The DATA, structures and algorithms!", true,
                                           &query_tokens, &query_count, error, sizeof(error)));
    TEST_ASSERT(query_count == 3U);
    TEST_ASSERT(strcmp(query_tokens[0], "data") == 0);
    TEST_ASSERT(strcmp(query_tokens[1], "structures") == 0);
    TEST_ASSERT(strcmp(query_tokens[2], "algorithms") == 0);
    tokenizer_free_tokens(query_tokens, query_count);

    (void)printf("PASS: tokenizer tests\n");
    return 0;
}
