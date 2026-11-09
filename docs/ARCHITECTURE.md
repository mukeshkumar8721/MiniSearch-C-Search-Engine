# MiniSearch Architecture

## 1. Architectural overview

MiniSearch follows a small pipeline:

1. The user chooses a corpus directory.
2. The document module finds regular `.txt` files directly in that directory.
3. The tokenizer reads each file and emits lowercase ASCII-alphanumeric words.
4. The index module stores each word in a hash table with document postings and token positions.
5. The search module tokenizes a query, evaluates AND/OR/phrase matching, and asks the ranking module to calculate scores.
6. The CLI prints sorted matches and index statistics.
7. The storage module can save or reload the complete index.

## 2. Module responsibilities

| Module | Files | Responsibility |
|---|---|---|
| Entry point | `src/main.c` | Parse the optional corpus argument, initialize the index, launch the CLI, and release resources |
| CLI | `src/cli.c` | Read menu input, call application operations, and display results |
| Utilities | `src/util.c`, `include/util.h` | String duplication, checked size multiplication, error messages, and dynamically sized input lines |
| Documents | `src/document.c`, `include/document.h` | Enumerate `.txt` paths and derive document names |
| Tokenizer | `src/tokenizer.c`, `include/tokenizer.h` | Stream tokens from files, normalize query text, and apply optional stop words |
| Index | `src/index.c`, `include/index.h` | Own document records, hash buckets, term entries, postings, and positions |
| Search | `src/search.c`, `include/search.h` | Implement AND, OR, phrase filtering, and result ordering |
| Ranking | `src/ranking.c`, `include/ranking.h` | Calculate normalized TF-IDF scores |
| Storage | `src/storage.c`, `include/storage.h` | Save and validate versioned binary index files |
| Timer | `src/timer.c`, `include/timer.h` | Provide elapsed-time measurements with platform-specific high-resolution clocks |

## 3. Core data model

`SearchIndex` owns:

- A dynamic array of `Document` records.
- A dynamic array of hash-table bucket pointers.
- The count of unique terms and indexed tokens.
- The stop-word setting used when the corpus was indexed.

Each `TermEntry` owns its normalized term string, a dynamic array of `Posting` structures, and a bucket-chain pointer. Each `Posting` identifies one document and owns a dynamic array of token positions. `frequency` equals `positions_count`.

Document IDs are assigned in sorted path order from zero. The posting arrays are therefore appended in ascending document-ID order during corpus construction and can be searched with binary search.

## 4. Ownership rules

- `SearchIndex` owns all document names and paths.
- Each hash entry owns its term string and posting array.
- Each posting owns its positions array.
- Search query token strings are owned by the query function and freed before it returns.
- `SearchResults` owns its result array; callers release it with `search_results_destroy`.
- `storage_load` constructs a separate temporary index. If any validation fails, the temporary index is destroyed and the caller's existing index remains unchanged.

## 5. Index build transaction

`index_build_corpus` creates a temporary `SearchIndex`, collects and sorts candidate paths, and adds documents to that temporary index. If a file cannot be opened, it is skipped and counted. If an allocation or read operation fails, the whole temporary build is discarded. Only after the build succeeds does the function destroy and replace the caller's current index.

This avoids leaving a partly rebuilt index in the normal failure path.

## 6. Portability

- The algorithm and data structures use C11 and the standard C library.
- macOS and Linux enumerate directories through `opendir`/`readdir` and inspect files with `stat`.
- Windows builds use `FindFirstFileA`/`FindNextFileA` from Win32.
- Elapsed time uses `clock_gettime(CLOCK_MONOTONIC)` where available, `QueryPerformanceCounter` on Windows, and `clock()` as a fallback.
- The storage format encodes integers explicitly as little-endian values rather than writing native structs or pointers.

Windows uses ANSI file APIs in this version; Unicode path support is a known limitation.
