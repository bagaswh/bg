#include "bg_slice.h"
#include "unity.h"
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "bg_common.h"
#include "bg_testutils.h"

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
        bg_expect_assertion(
            { BGSlice_new(int, 5, 0, NULL); }, "len=5; cap=0");
        bg_expect_assertion(
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
    ASSERT_SLICE_CAP(slice, 10);

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

    BGSlice_set_len(slice, 10);
    ASSERT_SLICE_LEN(slice, 10);
    TEST_ASSERT_EQUAL_MESSAGE(true, BGSlice_is_full(slice),
                              "slice is not full after setting len to 10");

    bg_expect_assertion(
        { BGSlice_set_len(slice, 20); }, "setting larger len than cap");

    BGSlice_free(slice);
}

typedef struct BGSlice_s BGSlice_s;
extern void BGSlice_incr_len(BGSlice_s *s, size_t incr);

// Test BGSlice_incr_len
void
test_BGSlice_incr_len(void)
{
    BGSlice *slice = BGSlice_new(int, 3, 10, NULL);
    TEST_ASSERT_NOT_NULL(slice);

    BGSlice_incr_len(slice, 2);
    ASSERT_SLICE_LEN(slice, 5);

    BGSlice_incr_len(slice, 5);
    ASSERT_SLICE_LEN(slice, 10);

    bg_expect_assertion(
        { BGSlice_incr_len(slice, 5); }, "increment past capacity");

    BGSlice_free(slice);
}

// Test BGSlice get capacity functionalities
void
test_BGSlice_get_cap_funcs(void)
{
    BGSlice *slice = BGSlice_new(int, 3, 10, NULL);
    TEST_ASSERT_NOT_NULL(slice);

    TEST_ASSERT_EQUAL(7, BGSlice_get_usable_cap(slice));
    TEST_ASSERT_EQUAL(10, BGSlice_get_cap(slice));

    BGSlice_set_len(slice, 8);
    TEST_ASSERT_EQUAL(2, BGSlice_get_usable_cap(slice));
    TEST_ASSERT_EQUAL(10, BGSlice_get_cap(slice));

    slice = BGSlice_append(slice, &(int) { 1 });
    slice = BGSlice_append(slice, &(int) { 2 });
    slice = BGSlice_append(slice, &(int) { 3 });
    slice = BGSlice_append(slice, &(int) { 4 });

    TEST_ASSERT_EQUAL(8, BGSlice_get_usable_cap(slice));
    TEST_ASSERT_EQUAL(20, BGSlice_get_cap(slice));

    BGSlice_free(slice);
}

///////////////////////
// Retrieval
//
void
test_BGSlice_get(void)
{
    bg_expect_assertion({ BGSlice_get(NULL, 0); }, "NULL slice");

    ///////////////////////
    // Slice has data
    //
    {
        int data[] = { 10, 20, 30, 40, 50 };
        BGSlice *slice =
            BGSlice_new_copy_from_buf(data, 5, 5, BG_SIZE_AUTO, NULL);
        TEST_ASSERT_NOT_NULL(slice);

        for (size_t i = 0; i < 5; i++) {
            int *ptr = (int *) BGSlice_get(slice, i);
            TEST_ASSERT_NOT_NULL(ptr);
            TEST_ASSERT_EQUAL(data[i], *ptr);
        }

        // Test out of bounds access (possibly segfault if bounds checking
        // is disabled)
        bg_expect_assertion({ BGSlice_get(slice, 5); }, "OOB access");

        BGSlice_free(slice);
    }

    ///////////////////////
    // Slice zero length
    //
    {
        BGSlice *slice = BGSlice_new(0, 0, sizeof(int), NULL);
        TEST_ASSERT_NOT_NULL(slice);
        bg_expect_assertion({ BGSlice_get_last(slice); }, "OOB access");
        BGSlice_free(slice);
    }
}

///////////////////////
// Slice copy, copy_from_buf, copy_from_slice
//
void
test_BGSlice_copy(void)
{
    {
        int src_data[] = { 1, 2, 3, 4, 5 };
        BGSlice *src =
            BGSlice_new_copy_from_buf(src_data, 5, 5, BG_SIZE_AUTO, NULL);
        BGSlice *dst = BGSlice_new(int, 5, 5, NULL);
        TEST_ASSERT_NOT_NULL(src);
        TEST_ASSERT_NOT_NULL(dst);

        ssize_t copied = BGSlice_copy(dst, src);
        TEST_ASSERT_EQUAL(5, copied);

        int *dst_data = (int *) BGSlice_get_data_ptr(dst);
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
            src_data, dst_data, 5 * sizeof(int),
            "slice data is not equal to src data");
        if (dst_data == src_data) {
            TEST_FAIL_MESSAGE("slice data ptr is equal to src data ptr");
        }

        BGSlice_free(src);
        BGSlice_free(dst);
    }

    {
        int src_data[] = { 1, 2, 3, 4, 5 };
        BGSlice *src = BGSlice_new_copy_from_buf(src_data, 5, 5, 5, NULL);
        BGSlice *dst = BGSlice_new(int, 3, 10, NULL);
        TEST_ASSERT_NOT_NULL(src);
        TEST_ASSERT_NOT_NULL(dst);

        ssize_t copied = BGSlice_copy(dst, src);
        TEST_ASSERT_EQUAL(3, copied); // min(dst->len, src->len)

        int *dst_data = (int *) BGSlice_get_data_ptr(dst);
        for (int i = 0; i < 3; i++) {
            TEST_ASSERT_EQUAL(src_data[i], dst_data[i]);
        }
        for (int i = 3; i < 5; i++) {
            TEST_ASSERT_NOT_EQUAL(src_data[i], dst_data[i]);
        }

        BGSlice_free(src);
        BGSlice_free(dst);
    }
}

extern BGSlice *BGSlice_grow_to_cap(BGSlice *s, size_t cap);

///////////////////////
// Growing
//
void
test_BGSlice_grow_to_cap(void)
{
    int data[] = { 1, 2, 3, 4, 5 };
    BGSlice *slice = BGSlice_new_copy_from_buf(data, 5, 5, 5, NULL);
    TEST_ASSERT_NOT_NULL(slice);
    ASSERT_SLICE_CAP(slice, 5);

    slice = BGSlice_grow_to_cap(slice, 10);
    TEST_ASSERT_NOT_NULL(slice);
    ASSERT_SLICE_CAP(slice, 10);
    ASSERT_SLICE_LEN(slice, 5);

    // the first 5 items should be the same as the original
    TEST_ASSERT_EQUAL_MEMORY(data, (int *) BGSlice_get_data_ptr(slice),
                             5 * sizeof(int));

    // Test growing to smaller capacity (should not change)
    bg_expect_assertion(
        { BGSlice_grow_to_cap(slice, 5); },
        "cannot grow slice to smaller capacity");
    ASSERT_SLICE_CAP(slice, 10); // should remain unchanged

    BGSlice_free(slice);
}

extern size_t BGSlice_new_cap(BGSlice *s);

void
test_BGSlice_new_cap(void)
{
    BGSlice *small_slice = BGSlice_new(int, 50, 100, NULL);
    TEST_ASSERT_NOT_NULL(small_slice);
    size_t new_cap = BGSlice_new_cap(small_slice);
    TEST_ASSERT_EQUAL(200, new_cap);
    BGSlice_free(small_slice);

    // Test large slice (>= 256)
    BGSlice *large_slice = BGSlice_new(int, 150, 300, NULL);
    TEST_ASSERT_NOT_NULL(small_slice);
    new_cap = BGSlice_new_cap(large_slice);
    TEST_ASSERT_EQUAL(375, new_cap); // 300 * 1.25
    BGSlice_free(large_slice);
}

///////////////////////
// Range
//
bool
sum_callback(void *item, size_t index, void *ctx)
{
    int *sum = (int *) ctx;
    int *value = (int *) item;
    *sum += *value;
    return true; // continue iteration
}

struct stop_at_index_ctx {
    size_t stop_index;
    size_t actual_stop_index;
};

bool
stop_at_index_callback(void *item, size_t index, void *ctx)
{
    struct stop_at_index_ctx *cctx = (struct stop_at_index_ctx *) ctx;
    if (index == cctx->stop_index) {
        cctx->actual_stop_index = index;
        return false; // stop when we reach the specified index
    }
    return true;
}

void
test_BGSlice_range_sum(void)
{
    int data[] = { 1, 2, 3, 4, 5 };
    BGSlice *slice =
        BGSlice_new_copy_from_buf(data, 5, 5, BG_SIZE_AUTO, NULL);
    TEST_ASSERT_NOT_NULL(slice);

    int sum = 0;
    BGSlice_range(slice, &sum, sum_callback);
    TEST_ASSERT_EQUAL(15, sum);

    BGSlice_free(slice);
}

void
test_BGSlice_range_early_stop(void)
{
    int data[] = { 1, 2, 3, 4, 5 };
    BGSlice *slice =
        BGSlice_new_copy_from_buf(data, 5, 5, BG_SIZE_AUTO, NULL);
    TEST_ASSERT_NOT_NULL(slice);

    struct stop_at_index_ctx stop_at = { 3, 0 };

    BGSlice_range(slice, &stop_at, stop_at_index_callback);

    TEST_ASSERT_EQUAL(3, stop_at.actual_stop_index);

    BGSlice_free(slice);
}

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

static bool
print_slice_item(void *item, size_t idx, void *ctx)
{
    int *value = (int *) item;
    printf("%d ", *value);
    return true;
}

///////////////////////
// append
//
void
test_BGSlice_append(void)
{
    BGSlice *slice = BGSlice_new(int, 0, 5, NULL);
    TEST_ASSERT_NOT_NULL(slice);
    ASSERT_SLICE_LEN(slice, 0);
    ASSERT_SLICE_CAP(slice, 5);

    BGSlice_append(slice, &(int) { 1 });
    BGSlice_range(slice, NULL, print_slice_item);
    printf("\n");
    ASSERT_SLICE_LEN(slice, 1);
    BGSlice_append(slice, &(int) { 2 });
    BGSlice_range(slice, NULL, print_slice_item);
    printf("\n");
    BGSlice_append(slice, &(int) { 3 });
    BGSlice_range(slice, NULL, print_slice_item);
    printf("\n");
    BGSlice_append(slice, &(int) { 4 });
    BGSlice_range(slice, NULL, print_slice_item);
    printf("\n");
    BGSlice_append(slice, &(int) { 5 });
    BGSlice_range(slice, NULL, print_slice_item);
    printf("\n");
    ASSERT_SLICE_LEN(slice, 5);
    BGSlice_append(slice, &(int) { 6 });
    BGSlice_range(slice, NULL, print_slice_item);
    printf("\n");
    ASSERT_SLICE_LEN(slice, 6);
    ASSERT_SLICE_CAP(slice, 10);

    int sum = 0;
    BGSlice_range(slice, &sum, sum_callback);
    TEST_ASSERT_EQUAL(21, sum);

    BGSlice_free(slice);
}

///////////////////////
// Sorting
//

extern enum BGStatus BGSlice_isort(BGSlice *s, void *ctx,
                                   BGSlice_sort_key_fn key_fn,
                                   BGSlice_sort_comparator comparator);

struct sort_test_ctx {
    int *prev;
    bool asc;
};

bool
validate_sorted(void *item, size_t idx, void *ctx)
{
    int *value = (int *) item;

    struct sort_test_ctx *cctx = (struct sort_test_ctx *) ctx;
    if (idx == 0) {
        cctx->prev = value;
        return true;
    }

    if (cctx->asc) {
        TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(
            *cctx->prev, *value,
            "current value is not greater than or equal to previous value");
    } else {
        TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(
            *cctx->prev, *value,
            "current value is not less or equal to previous value");
    }
    cctx->prev = value;
    return true;
}

struct qsort_mo3_partition_args {
    BGSlice_s *s;
    size_t low;
    size_t high;
    void *swap_tmp;
    size_t median_idx_result;
    BGSlice *pivot_indices;
};

enum BGStatus
BGSlice_qsort_mo3_partition(struct qsort_mo3_partition_args *args);

struct a {
    int age;
    char *name;
};

static bool
print_slice_item2(void *_item, size_t idx, void *ctx)
{
    struct a *item = (struct a *) _item;
    printf("age=%d name=%s\n", item->age, item->name);
    return true;
}

ssize_t
item2_comparator(BGSlice_s *s, comparable a, comparable b, void *ctx)
{
    struct a *aa = (struct a *) a;
    struct a *bb = (struct a *) b;

    int name_diff = strcmp(aa->name, bb->name);
    if (name_diff != 0) {
        return name_diff; // Ages are different, return the difference
    }

    // Ages are equal, compare by name
    return aa->age - bb->age;
}

void
test_BGSlice_sorting(void)
{
    // Unit test the insertion sort.

    // {
    //     int data[] = {
    //         7, 4, 1, 3, 8, 2, 2, 5, 1, 1441, 6, 8, 3, 7, 11, 331, 41,
    //     };

    //     BGSlice *slice_sort_asc = BGSlice_new_copy_from_buf(
    //         data, bg_arr_length(data), bg_arr_length(data), BG_SIZE_AUTO,
    //         NULL);
    //     TEST_ASSERT_NOT_NULL(slice_sort_asc);

    //     BGSlice_isort(slice_sort_asc, NULL, NULL, NULL);
    //     struct sort_test_ctx ctx_asc = { .prev = NULL, .asc = true };
    //     BGSlice_range(slice_sort_asc, &ctx_asc, validate_sorted);

    //     BGSlice *slice_sort_desc = BGSlice_new_copy_from_buf(
    //         data, bg_arr_length(data), bg_arr_length(data), BG_SIZE_AUTO,
    //         NULL);
    //     TEST_ASSERT_NOT_NULL(slice_sort_desc);

    //     BGSlice_isort(slice_sort_desc, NULL, NULL,
    //                   BGSlice_default_comparator_desc);
    //     struct sort_test_ctx ctx_desc = { .prev = NULL, .asc = false };
    //     BGSlice_range(slice_sort_desc, &ctx_desc, validate_sorted);
    // }

    // Unit test qsort
    // {
    //     int data[] = { 2, 8, 7, 1, 3, 5, 6, 4 };
    //     size_t arr_length = bg_arr_length(data);
    //     BGSlice *slice =
    //         BGSlice_new_from_buf(data, arr_length, BG_SIZE_AUTO, NULL);
    //     printf("BEFORE SORTED:\n");
    //     BGSlice_range(slice, NULL, print_slice_item);
    //     printf("\n");
    //     size_t pivot_idx;
    //     BGSlice_qsort(slice, NULL, NULL, NULL);
    //     printf("AFTER SORTED:\n");
    //     printf("\n");
    //     BGSlice_range(slice, NULL, print_slice_item);
    //     struct sort_test_ctx ctx_asc = { .prev = NULL, .asc = true };
    //     BGSlice_range(slice, &ctx_asc, validate_sorted);
    // }

    {
        struct a data[] = {
            { .age = 11, .name = "Salma" },  { .age = 9, .name = "Byzan" },
            { .age = 10, .name = "Bagas" },  { .age = 15, .name = "Dimas" },
            { .age = 99, .name = "Pihole" }, { .age = 35, .name = "Cybu" },
        };

        size_t arr_length = bg_arr_length(data);
        BGSlice *slice =
            BGSlice_new_from_buf(data, arr_length, BG_SIZE_AUTO, NULL);

        printf("BEFORE SORTED:\n");
        BGSlice_range(slice, NULL, print_slice_item2);
        printf("\n");
        BGSlice_qsort(slice, NULL, NULL, item2_comparator);
        printf("AFTER SORTED:\n");
        BGSlice_range(slice, NULL, print_slice_item2);
        printf("\n");
    }
}

typedef struct {
    void (*test_func)(void);
    const char *test_name;
} test_case_t;

static const test_case_t all_tests[] = {
    // { test_BGSlice_new, "test_BGSlice_new" },
    // { test_BGSlice_reset, "test_BGSlice_reset" },
    // { test_BGSlice_len_operations, "test_BGSlice_len_operations" },
    // { test_BGSlice_incr_len, "test_BGSlice_incr_len" },
    // { test_BGSlice_get_cap_funcs, "test_BGSlice_get_cap_funcs" },
    // { test_BGSlice_append, "test_BGSlice_append" },
    // { test_BGSlice_get, "test_BGSlice_get" },
    // { test_BGSlice_copy, "test_BGSlice_copy" },
    // { test_BGSlice_grow_to_cap, "test_BGSlice_grow_to_cap" },
    // { test_BGSlice_new_cap, "test_BGSlice_new_cap" },
    // { test_BGSlice_range_sum, "test_BGSlice_range_sum" },
    // { test_BGSlice_range_early_stop, "test_BGSlice_range_early_stop" },
    { test_BGSlice_sorting, "test_BGSlice_sorting" },
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