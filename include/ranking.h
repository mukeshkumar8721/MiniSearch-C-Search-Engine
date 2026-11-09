#ifndef MINISEARCH_RANKING_H
#define MINISEARCH_RANKING_H

#include "index.h"

/* Smoothed IDF: ln((N + 1) / (df + 1)) + 1. */
double ranking_idf(const SearchIndex *index, size_t document_frequency);
double ranking_term_score(const SearchIndex *index, size_t doc_id, const char *term);
double ranking_score_document(const SearchIndex *index,
                               size_t doc_id,
                               const char *const *unique_query_terms,
                               size_t query_term_count);

#endif
