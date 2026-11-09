#include "tokenizer.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool is_ascii_word_character(int character)
{
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9');
}

static char ascii_lower(int character)
{
    if (character >= 'A' && character <= 'Z') {
        return (char)(character - 'A' + 'a');
    }
    return (char)character;
}

TokenizerStatus tokenizer_next(FILE *stream, char **token_out)
{
    size_t capacity = 32U;
    size_t length = 0U;
    char *token;
    int character;

    if (stream == NULL || token_out == NULL) {
        return TOKENIZER_ERROR;
    }
    *token_out = NULL;
    token = (char *)malloc(capacity);
    if (token == NULL) {
        return TOKENIZER_ERROR;
    }

    do {
        character = fgetc(stream);
    } while (character != EOF && !is_ascii_word_character(character));

    if (character == EOF) {
        free(token);
        return ferror(stream) != 0 ? TOKENIZER_ERROR : TOKENIZER_EOF;
    }

    do {
        if (length + 1U >= capacity) {
            size_t new_capacity;
            char *grown;
            if (capacity > SIZE_MAX / 2U) {
                free(token);
                return TOKENIZER_ERROR;
            }
            new_capacity = capacity * 2U;
            grown = (char *)realloc(token, new_capacity);
            if (grown == NULL) {
                free(token);
                return TOKENIZER_ERROR;
            }
            token = grown;
            capacity = new_capacity;
        }
        token[length++] = ascii_lower(character);
        character = fgetc(stream);
    } while (character != EOF && is_ascii_word_character(character));

    if (character == EOF && ferror(stream) != 0) {
        free(token);
        return TOKENIZER_ERROR;
    }
    token[length] = '\0';
    *token_out = token;
    return TOKENIZER_TOKEN;
}

bool tokenizer_is_stop_word(const char *token)
{
    static const char *const stop_words[] = {
        "a", "an", "and", "are", "as", "at", "be", "but", "by",
        "for", "from", "in", "is", "it", "of", "on", "or", "the",
        "to", "was", "were", "with"
    };
    size_t i;

    if (token == NULL) {
        return false;
    }
    for (i = 0U; i < sizeof(stop_words) / sizeof(stop_words[0]); ++i) {
        if (strcmp(token, stop_words[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool tokenizer_tokenize_string(const char *text,
                               bool remove_stop_words,
                               char ***tokens_out,
                               size_t *count_out,
                               char *error_buffer,
                               size_t error_buffer_size)
{
    size_t capacity = 8U;
    size_t count = 0U;
    char **tokens;
    size_t position = 0U;

    if (text == NULL || tokens_out == NULL || count_out == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Invalid query tokenizer arguments.");
        return false;
    }
    *tokens_out = NULL;
    *count_out = 0U;
    tokens = (char **)malloc(capacity * sizeof(*tokens));
    if (tokens == NULL) {
        ms_set_error(error_buffer, error_buffer_size, "Out of memory while tokenizing query.");
        return false;
    }

    while (text[position] != '\0') {
        size_t start;
        size_t length;
        char *token;
        size_t i;
        while (text[position] != '\0' && !is_ascii_word_character((unsigned char)text[position])) {
            ++position;
        }
        if (text[position] == '\0') {
            break;
        }
        start = position;
        while (text[position] != '\0' && is_ascii_word_character((unsigned char)text[position])) {
            ++position;
        }
        length = position - start;
        token = (char *)malloc(length + 1U);
        if (token == NULL) {
            tokenizer_free_tokens(tokens, count);
            ms_set_error(error_buffer, error_buffer_size, "Out of memory while tokenizing query.");
            return false;
        }
        for (i = 0U; i < length; ++i) {
            token[i] = ascii_lower((unsigned char)text[start + i]);
        }
        token[length] = '\0';

        if (remove_stop_words && tokenizer_is_stop_word(token)) {
            free(token);
            continue;
        }
        if (count == capacity) {
            size_t new_capacity = capacity * 2U;
            char **grown;
            if (new_capacity < capacity || new_capacity > SIZE_MAX / sizeof(*tokens)) {
                free(token);
                tokenizer_free_tokens(tokens, count);
                ms_set_error(error_buffer, error_buffer_size, "Query contains too many tokens.");
                return false;
            }
            grown = (char **)realloc(tokens, new_capacity * sizeof(*tokens));
            if (grown == NULL) {
                free(token);
                tokenizer_free_tokens(tokens, count);
                ms_set_error(error_buffer, error_buffer_size, "Out of memory while tokenizing query.");
                return false;
            }
            tokens = grown;
            capacity = new_capacity;
        }
        tokens[count++] = token;
    }

    *tokens_out = tokens;
    *count_out = count;
    return true;
}

void tokenizer_free_tokens(char **tokens, size_t count)
{
    size_t i;
    if (tokens == NULL) {
        return;
    }
    for (i = 0U; i < count; ++i) {
        free(tokens[i]);
    }
    free(tokens);
}
