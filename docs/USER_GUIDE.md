# MiniSearch User Guide

## 1. Build

From the project root:

```sh
make
```

## 2. Start the program

Start with an empty index:

```sh
./build/minisearch
```

Or immediately index the sample documents:

```sh
./build/minisearch data/sample_documents
```

The optional directory argument indexes `.txt` files before the interactive menu appears. Stop-word filtering is disabled for this startup shortcut.

## 3. Index your own documents

1. Put plain-text `.txt` documents in a folder you control.
2. Start MiniSearch.
3. Choose option `1`.
4. Enter the folder path.
5. Choose whether to remove common English stop words.

The scanner examines only files directly inside that folder. It does not recurse into subfolders or scan arbitrary system directories. Unreadable files are skipped and counted.

Choose option `8` to rebuild the index from a selected folder.

## 4. Search

Choose option `2`, type a query, and choose a mode:

- `1`: AND — every distinct query word must be present.
- `2`: OR — at least one distinct query word must be present.
- `3`: exact phrase — the normalized query words must be consecutive and in the same order.

Example queries:

```text
data structures
binary search
C programming
search engine
```

Queries ignore ASCII case and treat punctuation as separators. If stop-word filtering was enabled during indexing, query stop words are filtered too.

The output includes the document name/path, matched query terms, relevance score, number of matching documents, and elapsed search time. At most ten results are printed.

## 5. View documents and statistics

- Choose option `3` to list document IDs, names, paths, and indexed word counts.
- Choose option `4` to view document count, unique term count, total indexed tokens, average document length, estimated memory use, and stop-word setting.
- Choose option `5` to view the ten most frequent indexed terms.

## 6. Save and load

Choose option `6` to save the current index. The default file path is `minisearch.idx`.

Choose option `7` to load an existing index. The loader checks file structure and content before replacing the current index. If validation fails, the existing in-memory index is retained.

The binary index includes the stop-word setting. Keep the file with the corpus if you want a saved searchable snapshot; the original source documents are not embedded in the index.

## 7. Benchmark

Choose option `10`, enter a corpus directory, and enter a query. The benchmark creates a separate temporary index, measures indexing and searching, and reports counts and estimated memory. It does not replace the index currently open in the interactive menu.

Record your computer, operating system, compiler, flags, corpus size, and dataset details before comparing measurements.

## 8. Exit and cleanup

Choose option `0` to exit. The program releases its in-memory index. Use:

```sh
make clean
```

to remove compiled output and test temporary directories. A saved `.idx` file is separate and should be removed manually if you no longer need it.
