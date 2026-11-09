# MiniSearch Learning Guide

This guide is designed to help a first-year CSE student explain the project rather than merely run it. Read one module, trace one example through it, and then make a small change yourself.

## Recommended reading order

### 1. `src/main.c` — program entry point

**Purpose:** initializes the index, optionally indexes a directory supplied on the command line, starts the menu, and frees memory before exit.

**C concepts:** `argc`/`argv`, function calls, return codes, structures, and cleanup.

**Trace:** run `./build/minisearch data/sample_documents`. Follow how `argv[1]` reaches `index_build_corpus`.

### 2. `src/cli.c` — user interaction

**Purpose:** presents menu choices and converts user input into calls to the engine modules.

**C concepts:** loops, `switch`, strings, `strtol`, dynamic input allocation, and `free`.

**Important function:** `search_documents` reads a query, selects AND/OR/phrase mode, calls `search_query`, and prints results.

**Debugging tip:** test empty input, letters where a menu number is expected, and end-of-file input.

### 3. `src/document.c` — document collection

**Purpose:** reads only `.txt` files immediately inside the chosen folder and returns sorted paths.

**C concepts:** pointers to arrays, `realloc`, string functions, directory streams, `stat`, and conditional compilation.

**Important functions:** `document_collect_txt_files`, `path_list_append`, and `document_basename_copy`.

**Debugging tip:** try a folder with no `.txt` files, one with an empty `.txt`, and one with a subfolder. The subfolder is not scanned.

### 4. `src/tokenizer.c` — text processing

**Purpose:** converts raw document bytes and query text into normalized tokens.

**C concepts:** character-by-character file reading, dynamic buffers, `malloc`, `realloc`, enums, and `free`.

**Important function:** `tokenizer_next` returns one of three statuses: token, end-of-file, or error.

**Trace:** the text `Search-Engine, C11` becomes `search`, `engine`, and `c11`.

**Debugging tip:** add a temporary print after each token is returned. Remove debug output before committing.

### 5. `src/index.c` — the inverted index

**Purpose:** owns documents and stores each term's document postings and positions.

**C concepts:** `struct`, pointers, hash functions, separate chaining, dynamic arrays, `calloc`, `realloc`, binary search, and ownership.

**Important functions:**

- `index_find_term`: finds a term's hash-table entry.
- `create_term`: allocates and inserts a new term.
- `add_occurrence`: appends an occurrence to the right posting.
- `index_build_corpus`: creates a complete temporary index and replaces the current one only when successful.

**Trace:** index `apple apple pear`. The `apple` posting should have frequency 2 and positions `[0, 1]`; `pear` should have frequency 1 and position `[2]`.

**Debugging tip:** inspect `posting->frequency`, `positions_count`, and the final position after each inserted word. They should remain consistent.

### 6. `src/search.c` — retrieval

**Purpose:** tokenizes a query and decides which documents satisfy the selected matching rule.

**C concepts:** arrays of pointers, binary search, loops, boolean logic, and `qsort`.

**Important functions:** `document_has_phrase`, `position_exists`, and `search_query`.

**Trace:** a phrase query `data structures` matches positions 3 and 4, but not positions 3 and 5.

**Debugging tip:** test AND and OR against three documents where each contains a different subset of the query terms.

### 7. `src/ranking.c` — relevance score

**Purpose:** calculates normalized TF-IDF.

**C concepts:** floating-point arithmetic, conversion from integer counts to `double`, mathematical functions, and function decomposition.

**Important functions:** `ranking_idf`, `ranking_term_score`, and `ranking_score_document`.

**Manual check:** with `N=2`, `df(apple)=2`, `df(banana)=1`, and a document `apple apple banana` of length 3:

```text
IDF(apple)  = ln(3/3) + 1 = 1
IDF(banana) = ln(3/2) + 1
score       = (2/3 × 1) + (1/3 × (ln(3/2) + 1))
```

### 8. `src/storage.c` — saving and loading

**Purpose:** saves the index as a versioned binary file and validates it on load.

**C concepts:** binary file streams, fixed-width integers, byte shifts, `fread`, `fwrite`, bounds checking, and transactional replacement.

**Important functions:** `storage_save`, `storage_load`, `write_u64`, `read_u64`, and `load_term`.

**Debugging tip:** save an index, load it into a fresh instance, and compare document counts, term counts, and query results. Then truncate a copy and check that loading fails without replacing the old in-memory index.

### 9. `src/timer.c` — measurement

**Purpose:** provides elapsed seconds from a monotonic or high-resolution timer when available.

**C concepts:** conditional compilation, platform APIs, and floating-point conversion.

**Debugging tip:** use the same corpus and query for repeated measurements. Do not compare numbers from different hardware without recording the environment.

## Major C concepts used

| Concept | Where it appears | What to understand |
|---|---|---|
| `struct` | `Document`, `Posting`, `TermEntry`, `SearchIndex` | Combine related fields into one type |
| Pointers | Index chains, dynamic arrays, function arguments | A pointer stores an address; it does not automatically own the target |
| `malloc` | Strings, positions, results | Allocates uninitialized memory; check for `NULL` |
| `calloc` | Hash buckets, records | Allocates and initializes memory to zero |
| `realloc` | Dynamic arrays and token buffers | May move the allocation; use a temporary pointer for the result |
| `free` | Destructors and cleanup paths | Each owned allocation must be released once |
| File I/O | Tokenizer and storage | Check open, read, write, and close results |
| Header guards | Every public header | Prevent repeated inclusion in one translation unit |
| `const` | Read-only inputs and lookup functions | Communicates which data a function does not modify |
| Enums | Tokenizer status and search mode | Give named values to a finite set of states |
| Function pointers/callbacks | Not required in this version | A possible future extension for tokenizer callbacks |
| Conditional compilation | Directory and timer modules | Select platform-specific implementation at compile time |
| `qsort` | Result and frequent-term ordering | Sorts an array using a comparison function |

## Memory ownership checklist

Before changing a function, ask:

1. Who allocates this pointer?
2. Which object owns it after the function returns?
3. Who frees it on success?
4. Who frees it on every failure path?
5. Could two owners accidentally free the same allocation?
6. Could a pointer be used after `realloc` moved the memory?

The `index_destroy` function is the main place to study nested ownership. It frees document strings, then each term entry, each posting array, and every positions array.

## A safe extension sequence

1. Add a test before changing an algorithm.
2. Make the smallest source change that should satisfy the test.
3. Run `make clean && make && make test`.
4. Review compiler warnings.
5. Run the sanitizer build if available.
6. Update the relevant documentation.
7. Commit one understandable change at a time.

## What this project does not teach yet

It does not yet implement Unicode normalization, multithreaded indexing, persistent caches, a network protocol, fuzzy search, a production query parser, or distributed search. Those are possible future topics after you can confidently explain the current pipeline.
