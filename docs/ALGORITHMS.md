# MiniSearch Algorithms

## 1. Tokenization and normalization

The tokenizer reads one byte at a time. ASCII letters and digits belong to a token. Every other byte separates tokens. Uppercase ASCII letters are converted to lowercase.

Example:

```text
Input:  "Search-Engines, C11!"
Tokens: "search", "engines", "c11"
```

This is intentionally a simple rule suitable for an introductory project. It does not implement Unicode-aware word boundaries.

If stop-word filtering is enabled, the tokenizer discards words in a small built-in English list such as `the`, `a`, `and`, `is`, and `of`. The same setting is applied to queries so that indexed terms and query terms use consistent processing.

## 2. Inverted index

A forward scan of the corpus emits a term for each accepted token. A hash table maps the term to its `TermEntry`. A term entry contains postings:

```text
term: structures
  document 0: frequency 2, positions [1, 7]
  document 3: frequency 1, positions [4]
```

The hash table uses separate chaining. Each posting list is a dynamic array. Since documents are processed in ascending ID order, postings are also in ascending document-ID order.

Expected hash lookup time is `O(1)` with a well-distributed hash function. The worst case is `O(U)` if many unique terms collide into one bucket, where `U` is the number of unique terms.

## 3. AND and OR retrieval

The query is tokenized and duplicate terms are removed for ordinary matching.

- AND returns a document only if every distinct query term has a posting for that document.
- OR returns a document if at least one distinct query term has a posting for that document.

The current implementation scans the document array and uses binary search in each term's posting list. If a term has `P` postings, one posting lookup costs `O(log P)`. For `N` documents and `Q` distinct query terms, the ordinary matching stage is approximately `O(N × Q × log P)`.

This implementation favors clarity. A later optimization could intersect the shortest posting lists rather than inspect every document.

## 4. Exact phrase search

Each occurrence has a zero-based position in the document's accepted token stream. For query tokens `q0, q1, ..., qk`, a phrase matches if there is a position `p` such that:

```text
p       occurs in q0's posting for the document
p + 1   occurs in q1's posting for the document
p + 2   occurs in q2's posting for the document
...
p + k   occurs in qk's posting for the document
```

Positions are searched with binary search. This makes phrase matching possible without loading and scanning the original file for every query.

If stop-word filtering is active, stop words are removed from both documents and queries before positions are assigned. Phrase semantics therefore apply to the filtered token sequence.

## 5. TF-IDF ranking

MiniSearch uses:

```text
TF(t, d) = frequency(t, d) / document_length(d)
IDF(t)   = ln((N + 1) / (df(t) + 1)) + 1
score(d) = Σ TF(t, d) × IDF(t)
```

`N` is the number of indexed documents. `df(t)` is the count of documents whose posting list contains `t`. The `+1` smoothing keeps the IDF positive and avoids a zero numerator/denominator edge case.

The score sums each distinct query term once. It is transparent and useful for demonstrating information-retrieval concepts, but it is not a production-grade ranking formula.

## 6. Sorting results

The result array is sorted with `qsort`, in descending score order. When scores are equal, the lower document ID comes first. This tie-break makes output deterministic for a fixed corpus ordering.

## 7. Index persistence

The file format stores a magic signature and version before any records. Integer values are written byte by byte in little-endian order. Strings are stored as a length followed by their bytes. The file contains no process-specific pointer values.

On load, MiniSearch validates bounds, ordered IDs, valid normalized terms, unique terms, sorted postings, position bounds, and aggregate token totals. It also checks that each document's total posting occurrences equal its indexed token count. The newly loaded index replaces the current index only after all checks pass.
