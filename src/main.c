#include "cli.h"
#include "index.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program_name)
{
    (void)printf("Usage: %s [corpus-directory]\n", program_name);
    (void)printf("\nWithout an argument, start the interactive menu.\n");
    (void)printf("With a directory argument, index its .txt files before opening the menu.\n");
    (void)printf("Example: %s data/sample_documents\n", program_name);
}

int main(int argc, char **argv)
{
    SearchIndex index;
    char error[256] = {0};
    int exit_code;

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc > 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (!index_init(&index, error, sizeof(error))) {
        (void)fprintf(stderr, "Startup failed: %s\n", error);
        return 1;
    }

    if (argc == 2) {
        size_t skipped = 0U;
        if (!index_build_corpus(&index, argv[1], false, &skipped, error, sizeof(error))) {
            (void)fprintf(stderr, "Could not index '%s': %s\n", argv[1], error);
            index_destroy(&index);
            return 1;
        }
        (void)printf("Indexed %zu document(s), %zu unique terms and %zu tokens.\n",
                     index.document_count, index.unique_terms, index.total_tokens);
        if (skipped > 0U) {
            (void)printf("Skipped %zu file(s) that could not be opened.\n", skipped);
        }
    }

    exit_code = cli_run(&index);
    index_destroy(&index);
    return exit_code;
}
