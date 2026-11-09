#ifndef MINISEARCH_TOKENIZER_H
#define MINISEARCH_TOKENIZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    TOKENIZER_ERROR = -1,
    TOKENIZER_EOF = 0,
    TOKENIZER_TOKEN = 1
} TokenizerStatus;

/* Returns one allocated, lowercase ASCII-alphanumeric token at a time. */
TokenizerStatus tokenizer_next(FILE *stream, char **token_out);
bool tokenizer_is_stop_word(const char *token);
bool tokenizer_tokenize_string(const char *text,
                               bool remove_stop_words,
                               char ***tokens_out,
                               size_t *count_out,
                               char *error_buffer,
                               size_t error_buffer_size);
void tokenizer_free_tokens(char **tokens, size_t count);

#endif
