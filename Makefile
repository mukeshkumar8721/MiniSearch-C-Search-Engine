CC ?= cc
AR ?= ar
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2
CPPFLAGS ?= -Iinclude
LDFLAGS ?=
LDLIBS ?= -lm
BUILD_DIR := build
CORE_SOURCES := src/util.c src/document.c src/tokenizer.c src/index.c src/ranking.c src/search.c src/storage.c src/timer.c
APP_SOURCES := $(CORE_SOURCES) src/cli.c src/main.c
TEST_NAMES := tokenizer index search ranking storage
TEST_BINS := $(addprefix $(BUILD_DIR)/test_,$(TEST_NAMES))

.PHONY: all clean test help

all: $(BUILD_DIR)/minisearch

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/minisearch: $(APP_SOURCES) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_%: tests/test_%.c $(CORE_SOURCES) tests/test_common.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Itests $(CFLAGS) tests/test_$*.c $(CORE_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

test: $(TEST_BINS)
	@set -e; for test_binary in $(TEST_BINS); do \
	  echo "==> $$test_binary"; \
	  ./$$test_binary; \
	done

clean:
	rm -rf $(BUILD_DIR) tests/tmp_*

help:
	@echo "make          Build the MiniSearch application"
	@echo "make test     Build and run the automated tests"
	@echo "make clean    Remove generated build/test files"
