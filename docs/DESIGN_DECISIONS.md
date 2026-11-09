# MiniSearch Design Decisions

## 1. C11 and standard-library-first implementation

C11 is sufficient for the core application and keeps the project focused on systems fundamentals. Platform-specific directory and timer APIs are isolated behind small modules. No external search API or indexing library is used.

## 2. Hash table with separate chaining

The inverted index needs frequent term lookup during corpus construction and searching. A hash table provides expected constant-time lookup. Separate chaining makes collisions manageable without requiring a probing/deletion scheme. The bucket array grows when the term count reaches roughly 75% of the bucket count.

## 3. Dynamic posting arrays

Posting lists are read often and iterated in order. Dynamic arrays provide good locality and straightforward binary search by document ID. A linked list would make random access slower and require one allocation per node.

## 4. Positional postings

A frequency alone cannot establish whether query terms occur consecutively. Each posting therefore stores the positions of all accepted term occurrences. Frequency is kept explicitly and maintained equal to the number of positions.

## 5. Document array

Document IDs are assigned sequentially and remain stable for the lifetime of an index. A dynamic array maps an ID to its path, name, and token count in constant time.

## 6. ASCII tokenizer

A byte-oriented ASCII tokenizer makes behavior easy to explain and test. It avoids claiming Unicode support that has not been implemented. This is a deliberate scope decision, not a claim that ASCII-only tokenization is sufficient for every language.

## 7. Smoothed, normalized TF-IDF

Term frequency is divided by document length so long documents do not automatically dominate only because they contain more words. IDF uses `ln((N + 1) / (df + 1)) + 1`. The formula is simple, deterministic, and easy to verify manually.

## 8. Transactional rebuild and load

A rebuild or load is constructed in a temporary `SearchIndex`. The existing index is replaced only when the new index is complete. This protects the current in-memory data from common failures during indexing or parsing.

## 9. Explicit binary serialization

The storage module writes integer fields byte by byte in little-endian order and stores lengths before variable-length data. It does not write C structs directly because structure padding, native endianness, and pointers are not portable file data.

## 10. Simplicity over premature optimization

Query evaluation currently examines each document. That is not the fastest strategy for very large corpora, but it makes AND/OR semantics easy for a first-year student to follow. Posting-list intersection can be added after correctness and complexity are understood.
