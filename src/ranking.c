#include "ranking.h"

#include <math.h>
#include <string.h>

double ranking_idf(const SearchIndex *index, size_t document_frequency)
{
    double document_count;
    double frequency;

    if (index == NULL) {
        return 0.0;
    }
    document_count = (double)index->document_count;
    frequency = (double)document_frequency;
    return log((document_count + 1.0) / (frequency + 1.0)) + 1.0;
}

double ranking_term_score(const SearchIndex *index, size_t doc_id, const char *term)
{
    const TermEntry *entry;
    const Posting *posting;
    size_t document_length;
    double term_frequency;

    if (index == NULL || term == NULL || doc_id >= index->document_count) {
        return 0.0;
    }
    entry = index_find_term(index, term);
    posting = index_get_posting(entry, doc_id);
    if (entry == NULL || posting == NULL) {
        return 0.0;
    }
    document_length = index->documents[doc_id].word_count;
    if (document_length == 0U) {
        return 0.0;
    }

    term_frequency = (double)posting->frequency / (double)document_length;
    return term_frequency * ranking_idf(index, entry->posting_count);
}

double ranking_score_document(const SearchIndex *index,
                               size_t doc_id,
                               const char *const *unique_query_terms,
                               size_t query_term_count)
{
    double score = 0.0;
    size_t i;

    if (index == NULL || unique_query_terms == NULL || doc_id >= index->document_count) {
        return 0.0;
    }
    for (i = 0U; i < query_term_count; ++i) {
        size_t previous;
        bool duplicate = false;
        for (previous = 0U; previous < i; ++previous) {
            if (unique_query_terms[previous] != NULL && unique_query_terms[i] != NULL &&
                strcmp(unique_query_terms[previous], unique_query_terms[i]) == 0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            score += ranking_term_score(index, doc_id, unique_query_terms[i]);
        }
    }
    return score;
}
