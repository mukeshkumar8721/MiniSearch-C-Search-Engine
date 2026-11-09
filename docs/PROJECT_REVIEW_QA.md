# MiniSearch — Project Review Questions and Answers

## 1. What is an inverted index?

An inverted index maps each word to the documents that contain it. Instead of scanning every document for every query, MiniSearch looks up a term and checks its posting list.

## 2. Why did you use a hash table?

The engine needs frequent term lookup while indexing and searching. A hash table gives expected constant-time lookup when its hash function distributes terms well. Separate chaining handles collisions.

## 3. What information is stored in a posting?

A posting stores a document ID, the frequency of the term in that document, and every token position where the term occurs. The positions make exact phrase matching possible.

## 4. Why are the posting lists dynamic arrays?

They grow as documents are indexed, use contiguous memory, and can be searched by document ID with binary search. The trade-off is that growing an array can require reallocating and copying it.

## 5. How does phrase search work?

For each position of the first query term, the engine checks whether the second term appears at the next position, the third at the following position, and so on. A phrase matches only when all positions are consecutive and ordered.

## 6. Explain the TF-IDF formula used in this project.

Term frequency is the term's count divided by the indexed document length. Inverse document frequency is `ln((N + 1) / (df + 1)) + 1`. The document score is the sum of TF multiplied by IDF over distinct query terms. The smoothing keeps the IDF positive and avoids division by zero.

## 7. What is the difference between AND and OR search?

AND requires every distinct query term to be present in the document. OR requires at least one of the distinct query terms to be present.

## 8. How does MiniSearch avoid memory leaks?

Every dynamic allocation has an owner and cleanup path. `index_destroy` releases nested index allocations. Search results have a dedicated destructor. Temporary indexes are destroyed on failed builds or failed loads, and the CLI frees input strings after use.

## 9. Why does the storage file contain a version and explicit integer fields?

The version identifies the serialization layout so future formats can be distinguished. Integers are written byte by byte in little-endian order, avoiding platform-specific struct padding and native endianness. Pointer values are never serialized.

## 10. What are the current limitations, and how would you improve the project?

The tokenizer is ASCII-only, directory scanning is non-recursive, and queries support only AND, OR, and exact phrase modes. For larger corpora, I would first optimize query evaluation by intersecting the shortest posting lists, then add Unicode-aware tokenization and more comprehensive benchmarks with a documented dataset.
