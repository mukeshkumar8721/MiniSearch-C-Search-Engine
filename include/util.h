#ifndef MINISEARCH_UTIL_H
#define MINISEARCH_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

char *ms_strdup(const char *text);
bool ms_size_multiply(size_t a, size_t b, size_t *result);
void ms_set_error(char *buffer, size_t buffer_size, const char *format, ...);
bool ms_read_line(FILE *stream, char **line_out);
void ms_trim_whitespace(char *text);

#endif
