CC      := clang
CFLAGS  := -Wall -Wextra -Werror -pedantic -Wno-newline-eof -std=c23 
LDFLAGS :=

INCLUDES := -I./src -I./third_party/Unity/src

ASAN_FLAGS := -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-return=always -fsanitize-address-use-after-scope
DEBUG_FLAGS := -g -O0
TEST_FLAGS := $(ASAN_FLAGS) $(DEBUG_FLAGS)
TEST_ASAN_ENV := ASAN_OPTIONS=detect_leaks=0

SRC_DIR     := src/container
UNITY_DIR   := third_party/Unity/src

UNITY_OBJ   := build/unity.o
SLICE_OBJ   := build/bg_slice.o
SLICE_TEST  := build/bg_slice_test
SLICE_TEST_DEBUG  := build/bg_slice_test_dbg
TEST_MACROS	:= -D__BG_RUNNING_TEST__ 

.PHONY: all debug clean test

debug: CFLAGS += $(DEBUG_FLAGS)
debug: LDFLAGS += $(DEBUG_FLAGS)
debug: all

$(UNITY_OBJ): $(UNITY_DIR)/unity.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(SRC_DIR)/bg_slice.c src/container/bg_slice_test.c $(UNITY_OBJ)
	@mkdir -p $(@D)
	$(CC) $(TEST_FLAGS) $(INCLUDES) $(TEST_MACROS) $^ -o $(SLICE_TEST) $(LDFLAGS)
	$(TEST_ASAN_ENV) ./$(SLICE_TEST)

test-debug: $(SRC_DIR)/bg_slice.c src/container/bg_slice_test.c $(UNITY_OBJ)
	@mkdir -p $(@D)
	$(CC) $(DEBUG_FLAGS) $(INCLUDES) $(TEST_MACROS) $^ -o $(SLICE_TEST_DEBUG) $(LDFLAGS)
	./$(SLICE_TEST_DEBUG)

clean:
	rm -rf build
