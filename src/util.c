#include "util.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *ms_strdup(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL) {
        return NULL;
    }

    length = strlen(text);
    if (length == SIZE_MAX) {
        return NULL;
    }

    copy = (char *)malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length + 1U);
    return copy;
}

bool ms_size_multiply(size_t a, size_t b, size_t *result)
{
    if (result == NULL || (b != 0U && a > SIZE_MAX / b)) {
        return false;
    }
    *result = a * b;
    return true;
}

void ms_set_error(char *buffer, size_t buffer_size, const char *format, ...)
{
    va_list args;

    if (buffer == NULL || buffer_size == 0U || format == NULL) {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
}

bool ms_read_line(FILE *stream, char **line_out)
{
    size_t capacity = 128U;
    size_t length = 0U;
    char *line;
    int character;

    if (stream == NULL || line_out == NULL) {
        return false;
    }
    *line_out = NULL;

    line = (char *)malloc(capacity);
    if (line == NULL) {
        return false;
    }

    while ((character = fgetc(stream)) != EOF) {
        if (character == '\n') {
            break;
        }
        if (length + 1U >= capacity) {
            size_t new_capacity;
            char *grown;

            if (capacity > SIZE_MAX / 2U) {
                free(line);
                return false;
            }
            new_capacity = capacity * 2U;
            grown = (char *)realloc(line, new_capacity);
            if (grown == NULL) {
                free(line);
                return false;
            }
            line = grown;
            capacity = new_capacity;
        }
        line[length++] = (char)character;
    }

    if (character == EOF && ferror(stream) != 0) {
        free(line);
        return false;
    }
    if (character == EOF && length == 0U) {
        free(line);
        return false;
    }

    if (length > 0U && line[length - 1U] == '\r') {
        --length;
    }
    line[length] = '\0';
    *line_out = line;
    return true;
}

void ms_trim_whitespace(char *text)
{
    size_t start = 0U;
    size_t end;

    if (text == NULL) {
        return;
    }

    while (text[start] == ' ' || text[start] == '\t' || text[start] == '\r' || text[start] == '\n') {
        ++start;
    }
    end = strlen(text);
    while (end > start &&
           (text[end - 1U] == ' ' || text[end - 1U] == '\t' ||
            text[end - 1U] == '\r' || text[end - 1U] == '\n')) {
        --end;
    }

    if (start > 0U) {
        memmove(text, text + start, end - start);
    }
    text[end - start] = '\0';
}
