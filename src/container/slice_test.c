#include "slice.h"
#include "unity.h"
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// Test helper macros
#define ASSERT_SLICE_LEN(slice, expected) \
    TEST_ASSERT_EQUAL(expected, BGSlice_get_len(slice))
#define ASSERT_SLICE_CAP(slice, expected) \
    TEST_ASSERT_EQUAL(expected, BGSlice_get_cap(slice))

void
setUp(void)
{
    // Called before each test
}

void
tearDown(void)
{
    // Called after each test
}

#define expect_assertion(stuff_to_do, name)                               \
    do {                                                                  \
        volatile int assertions_run = 0;                                  \
        if (setjmp(test_resume_env) == 0) {                               \
            stuff_to_do                                                   \
        } else {                                                          \
            assertions_run++;                                             \
        }                                                                 \
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, assertions_run,                  \
                                      "Assertion did not run for " name); \
    } while (0)

// Used to test if assertion works.
jmp_buf test_resume_env;

// Test BGSlice_new
void
test_BGSlice_new(void)
{
    // valid params
    {
        BGSlice *slice = BGSlice_new(int, 5, 10, NULL);
        TEST_ASSERT_NOT_NULL(slice);
        TEST_ASSERT_NOT_NULL(BGSlice_get_data_ptr(slice));
        ASSERT_SLICE_LEN(slice, 5);
        ASSERT_SLICE_CAP(slice, 10);
    }

    // invalid params
    {
        expect_assertion({ BGSlice_new(int, 5, 0, NULL); }, "len=5; cap=0");
        expect_assertion(
            { __BGSlice_new(5, 10, 0, NULL); },
            "zero elem_size"); // zero elem_size
    }

    // new from existing buffer
    {
        const char *test_str = "hello";
        size_t test_str_len = strlen(test_str);
        BGSlice *slice = BGSlice_new_copy_from_buf(
            test_str, test_str_len, BG_SIZE_AUTO, BG_SIZE_AUTO, NULL);
        TEST_ASSERT_NOT_NULL(slice);
        ASSERT_SLICE_LEN(slice, 0);
        ASSERT_SLICE_CAP(slice, test_str_len);
        TEST_ASSERT_EQUAL_MEMORY(
            test_str, (char *) BGSlice_get_data_ptr(slice), test_str_len);
        BGSlice_free(slice);
    }

    {
        const char *test_str = "";
        size_t test_str_len = strlen(test_str);
        BGSlice *slice = BGSlice_new_copy_from_buf(
            test_str, test_str_len, BG_SIZE_AUTO, BG_SIZE_AUTO, NULL);
        TEST_ASSERT_NOT_NULL(slice);
        ASSERT_SLICE_LEN(slice, 0);
        ASSERT_SLICE_CAP(slice, test_str_len);
        BGSlice_free(slice);
    }
}

// Test BGSlice_reset
void
test_BGSlice_reset(void)
{
    BGSlice *slice = BGSlice_new(int, 5, 10, NULL);
    TEST_ASSERT_NOT_NULL(slice);
    ASSERT_SLICE_LEN(slice, 5);

    BGSlice_reset(slice);
    ASSERT_SLICE_LEN(slice, 0);
    ASSERT_SLICE_CAP(slice, 10); // capacity should remain unchanged

    BGSlice_free(slice);
}

// Test BGSlice_get_len and BGSlice_set_len
void
test_BGSlice_len_operations(void)
{
    BGSlice *slice = BGSlice_new(int, 3, 10, NULL);
    TEST_ASSERT_NOT_NULL(slice);

    ASSERT_SLICE_LEN(slice, 3);

    BGSlice_set_len(slice, 7);
    ASSERT_SLICE_LEN(slice, 7);

    expect_assertion(
        { BGSlice_set_len(slice, 20); }, "setting larger len than cap");

    BGSlice_free(slice);
}

// // Test BGSlice_incr_len
// void
// test_BGSlice_incr_len(void)
// {
//     struct BGSlice *slice = BGSlice_new(10, 3, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     BGSlice_incr_len(slice, 2);
//     ASSERT_SLICE_LEN(slice, 5);

//     BGSlice_incr_len(slice, 5);
//     ASSERT_SLICE_LEN(slice, 10);

//     BGSlice_free(slice);
// }

// // Test BGSlice_get_usable_cap
// void
// test_BGSlice_get_usable_cap(void)
// {
//     struct BGSlice *slice = BGSlice_new(10, 3, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     TEST_ASSERT_EQUAL(7, BGSlice_get_usable_cap(slice));

//     BGSlice_set_len(slice, 8);
//     TEST_ASSERT_EQUAL(2, BGSlice_get_usable_cap(slice));

//     BGSlice_free(slice);
// }

// Test BGSlice_append
// void
// test_BGSlice_append(void)
// {
//     struct BGSlice *slice = BGSlice_new(5, 0, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     int value1 = 42;
//     int value2 = 84;

//     slice = BGSlice_append(slice, &value1);
//     ASSERT_SLICE_NOT_NULL(slice);
//     ASSERT_SLICE_LEN(slice, 1);

//     slice = BGSlice_append(slice, &value2);
//     ASSERT_SLICE_NOT_NULL(slice);
//     ASSERT_SLICE_LEN(slice, 2);

//     int *data = (int *) BGSlice_get_data_ptr(slice);
//     TEST_ASSERT_EQUAL(42, data[0]);
//     TEST_ASSERT_EQUAL(84, data[1]);

//     BGSlice_free(slice);
// }

// Test BGSlice_append with growth
// void
// test_BGSlice_append_with_growth(void)
// {
//     struct BGSlice *slice = BGSlice_new(2, 0, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     int values[] = { 1, 2, 3, 4 };

//     for (int i = 0; i < 4; i++) {
//         slice = BGSlice_append(slice, &values[i]);
//         ASSERT_SLICE_NOT_NULL(slice);
//     }

//     ASSERT_SLICE_LEN(slice, 4);
//     TEST_ASSERT_GREATER_OR_EQUAL(4, BGSlice_get_cap(slice));

//     int *data = (int *) BGSlice_get_data_ptr(slice);
//     for (int i = 0; i < 4; i++) {
//         TEST_ASSERT_EQUAL(values[i], data[i]);
//     }

//     BGSlice_free(slice);
// }

// Test BGSlice_get
// void
// test_BGSlice_get(void)
// {
//     int data[] = { 10, 20, 30, 40, 50 };
//     struct BGSlice *slice =
//         BGSlice_copy_from_buf(data, 5, 5, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     int *ptr = (int *) BGSlice_get(slice, 0);
//     TEST_ASSERT_NOT_NULL(ptr);
//     TEST_ASSERT_EQUAL(10, *ptr);

//     ptr = (int *) BGSlice_get(slice, 2);
//     TEST_ASSERT_NOT_NULL(ptr);
//     TEST_ASSERT_EQUAL(30, *ptr);

//     ptr = (int *) BGSlice_get(slice, 4);
//     TEST_ASSERT_NOT_NULL(ptr);
//     TEST_ASSERT_EQUAL(50, *ptr);

//     // Test out of bounds access (should return NULL if bounds checking is
//     // disabled)
//     ptr = (int *) BGSlice_get(slice, 5);
//     TEST_ASSERT_NULL(ptr);

//     BGSlice_free(slice);
// }

// Test BGSlice_get_last
// void
// test_BGSlice_get_last(void)
// {
//     int data[] = { 10, 20, 30 };
//     struct BGSlice *slice =
//         BGSlice_copy_from_buf(data, 5, 3, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     int *last = (int *) BGSlice_get_last(slice);
//     TEST_ASSERT_NOT_NULL(last);
//     TEST_ASSERT_EQUAL(30, *last);

//     BGSlice_free(slice);
// }

// void
// test_BGSlice_get_last_empty_slice(void)
// {
//     struct BGSlice *slice = BGSlice_new(5, 0, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     int *last = (int *) BGSlice_get_last(slice);
//     TEST_ASSERT_NULL(last);

//     BGSlice_free(slice);
// }

// Test BGSlice_is_full
// void
// test_BGSlice_is_full(void)
// {
//     struct BGSlice *slice = BGSlice_new(3, 2, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     TEST_ASSERT_FALSE(BGSlice_is_full(slice));

//     BGSlice_set_len(slice, 3);
//     TEST_ASSERT_TRUE(BGSlice_is_full(slice));

//     BGSlice_free(slice);
// }

// Test BGSlice_copy
// void
// test_BGSlice_copy(void)
// {
//     int src_data[] = { 1, 2, 3, 4, 5 };
//     struct BGSlice *src =
//         BGSlice_copy_from_buf(src_data, 5, 5, sizeof(int), NULL);
//     struct BGSlice *dst = BGSlice_new(5, 5, sizeof(int), NULL);

//     ASSERT_SLICE_NOT_NULL(src);
//     ASSERT_SLICE_NOT_NULL(dst);

//     ssize_t copied = BGSlice_copy(dst, src);
//     TEST_ASSERT_EQUAL(5, copied);

//     int *dst_data = (int *) BGSlice_get_data_ptr(dst);
//     for (int i = 0; i < 5; i++) {
//         TEST_ASSERT_EQUAL(src_data[i], dst_data[i]);
//     }

//     BGSlice_free(src);
//     BGSlice_free(dst);
// }

// // Test BGSlice_copy with different sizes
// void
// test_BGSlice_copy_different_sizes(void)
// {
//     int src_data[] = { 1, 2, 3, 4, 5 };
//     struct BGSlice *src =
//         BGSlice_copy_from_buf(src_data, 5, 5, sizeof(int), NULL);
//     struct BGSlice *dst = BGSlice_new(10, 3, sizeof(int), NULL);

//     ASSERT_SLICE_NOT_NULL(src);
//     ASSERT_SLICE_NOT_NULL(dst);

//     ssize_t copied = BGSlice_copy(dst, src);
//     TEST_ASSERT_EQUAL(3, copied); // min(dst->len, src->len)

//     int *dst_data = (int *) BGSlice_get_data_ptr(dst);
//     for (int i = 0; i < 3; i++) {
//         TEST_ASSERT_EQUAL(src_data[i], dst_data[i]);
//     }

//     BGSlice_free(src);
//     BGSlice_free(dst);
// }

// // Test BGSlice_grow_to_cap
// void
// test_BGSlice_grow_to_cap(void)
// {
//     struct BGSlice *slice = BGSlice_new(5, 3, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);
//     ASSERT_SLICE_CAP(slice, 5);

//     slice = BGSlice_grow_to_cap(slice, 10);
//     ASSERT_SLICE_NOT_NULL(slice);
//     ASSERT_SLICE_CAP(slice, 10);
//     ASSERT_SLICE_LEN(slice, 3); // length should remain unchanged

//     // Test growing to smaller capacity (should not change)
//     slice = BGSlice_grow_to_cap(slice, 5);
//     ASSERT_SLICE_NOT_NULL(slice);
//     ASSERT_SLICE_CAP(slice, 10); // should remain unchanged

//     BGSlice_free(slice);
// }

// // Test new_cap calculation
// void
// test_BGSlice_new_cap(void)
// {
//     // Test small slice (< 256)
//     struct BGSlice *small_slice = BGSlice_new(100, 50, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(small_slice);
//     size_t new_cap = BGSlice_new_cap(small_slice);
//     TEST_ASSERT_EQUAL(200, new_cap); // 100 * 2
//     BGSlice_free(small_slice);

//     // Test large slice (>= 256)
//     struct BGSlice *large_slice = BGSlice_new(300, 150, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(large_slice);
//     new_cap = BGSlice_new_cap(large_slice);
//     TEST_ASSERT_EQUAL(375, new_cap); // 300 * 1.25
//     BGSlice_free(large_slice);
// }

// // Range callback for testing
// bool
// sum_callback(void *item, size_t index, void *ctx)
// {
//     int *sum = (int *) ctx;
//     int *value = (int *) item;
//     *sum += *value;
//     return true; // continue iteration
// }

// bool
// stop_at_index_callback(void *item, size_t index, void *ctx)
// {
//     size_t *stop_index = (size_t *) ctx;
//     return index < *stop_index; // stop when we reach the specified index
// }

// // Test BGSlice_range
// void
// test_BGSlice_range_sum(void)
// {
//     int data[] = { 1, 2, 3, 4, 5 };
//     struct BGSlice *slice =
//         BGSlice_copy_from_buf(data, 5, 5, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     int sum = 0;
//     BGSlice_range(slice, &sum, sum_callback);
//     TEST_ASSERT_EQUAL(15, sum); // 1+2+3+4+5

//     BGSlice_free(slice);
// }

// void
// test_BGSlice_range_early_stop(void)
// {
//     int data[] = { 1, 2, 3, 4, 5 };
//     struct BGSlice *slice =
//         BGSlice_copy_from_buf(data, 5, 5, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     size_t stop_at = 3;
//     int sum = 0;

//     // Custom callback that sums and stops at index 3
//     BGSlice_range(slice, &stop_at, stop_at_index_callback);

//     // We can't easily test the early stop without modifying the callback,
//     // but we can test that the function doesn't crash with early return
//     TEST_ASSERT_TRUE(true); // Just verify no crash occurred

//     BGSlice_free(slice);
// }

// Test with custom allocator option (if needed)
// void
// test_BGSlice_with_null_option(void)
// {
//     struct BGSlice *slice = BGSlice_new(5, 3, sizeof(int), NULL);
//     ASSERT_SLICE_NOT_NULL(slice);
//     ASSERT_SLICE_CAP(slice, 5);
//     ASSERT_SLICE_LEN(slice, 3);
//     BGSlice_free(slice);
// }

// Test char slice operations
// void
// test_BGSlice_char_operations(void)
// {
//     struct BGSlice *slice = BGSlice_char_new(10, 0, NULL);
//     ASSERT_SLICE_NOT_NULL(slice);

//     char c1 = 'A';
//     char c2 = 'B';

//     slice = BGSlice_append(slice, &c1);
//     ASSERT_SLICE_NOT_NULL(slice);
//     slice = BGSlice_append(slice, &c2);
//     ASSERT_SLICE_NOT_NULL(slice);

//     ASSERT_SLICE_LEN(slice, 2);

//     char *data = (char *) BGSlice_get_data_ptr(slice);
//     TEST_ASSERT_EQUAL('A', data[0]);
//     TEST_ASSERT_EQUAL('B', data[1]);

//     BGSlice_free(slice);
// }

typedef struct {
    void (*test_func)(void);
    const char *test_name;
} test_case_t;

static const test_case_t all_tests[] = {
    { test_BGSlice_new, "test_BGSlice_new" },
    { test_BGSlice_reset, "test_BGSlice_reset" },
    { test_BGSlice_len_operations, "test_BGSlice_len_operations" },
    // { test_BGSlice_incr_len, "test_BGSlice_incr_len" },
    // { test_BGSlice_get_usable_cap, "test_BGSlice_get_usable_cap" },
    // { test_BGSlice_append, "test_BGSlice_append" },
    // { test_BGSlice_append_with_growth, "test_BGSlice_append_with_growth" },
    // { test_BGSlice_get, "test_BGSlice_get" },
    // { test_BGSlice_get_last, "test_BGSlice_get_last" },
    // { test_BGSlice_get_last_empty_slice,
    //   "test_BGSlice_get_last_empty_slice" },
    // { test_BGSlice_is_full, "test_BGSlice_is_full" },
    // { test_BGSlice_copy, "test_BGSlice_copy" },
    // { test_BGSlice_copy_different_sizes,
    //   "test_BGSlice_copy_different_sizes" },
    // { test_BGSlice_grow_to_cap, "test_BGSlice_grow_to_cap" },
    // { test_BGSlice_new_cap, "test_BGSlice_new_cap" },
    // { test_BGSlice_range_sum, "test_BGSlice_range_sum" },
    // { test_BGSlice_range_early_stop, "test_BGSlice_range_early_stop" },
    // { test_BGSlice_with_null_option, "test_BGSlice_with_null_option" },
    // { test_BGSlice_char_operations, "test_BGSlice_char_operations" }
};

#define NUM_TESTS (sizeof(all_tests) / sizeof(all_tests[0]))

void
run_all_tests(void)
{
    for (size_t i = 0; i < NUM_TESTS; i++) {
        UnityDefaultTestRun(all_tests[i].test_func, all_tests[i].test_name,
                            __LINE__);
    }
}

void
run_single_test(const char *test_name)
{
    for (size_t i = 0; i < NUM_TESTS; i++) {
        if (strcmp(all_tests[i].test_name, test_name) == 0) {
            UnityDefaultTestRun(all_tests[i].test_func,
                                all_tests[i].test_name, __LINE__);
            return;
        }
    }
    printf("Test '%s' not found!\n", test_name);
}

void
run_test_by_index(size_t index)
{
    if (index < NUM_TESTS) {
        UnityDefaultTestRun(all_tests[index].test_func,
                            all_tests[index].test_name, __LINE__);
    } else {
        printf("Test index %zu out of range (0-%zu)!\n", index,
               NUM_TESTS - 1);
    }
}

void
list_all_tests(void)
{
    printf("Available tests (%zu total):\n", NUM_TESTS);
    for (size_t i = 0; i < NUM_TESTS; i++) {
        printf("  [%2zu] %s\n", i, all_tests[i].test_name);
    }
}

int
main(int argc, char *argv[])
{
    if (argc == 1) {
        run_all_tests();
    } else if (argc == 2) {
        if (strcmp(argv[1], "--list") == 0) {
            list_all_tests();
        } else {
            // Try to parse as index first
            char *endptr;
            unsigned long index = strtoul(argv[1], &endptr, 10);
            if (*endptr == '\0') {
                run_test_by_index((size_t) index);
            } else {
                run_single_test(argv[1]);
            }
        }
    } else {
        printf("Usage: %s [test_name|test_index|--list]\n", argv[0]);
        return 1;
    }

    return 0;
}