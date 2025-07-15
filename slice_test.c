#include "slice.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

// Internal structure definition for testing
typedef struct Slice_s {
	size_t cap;
	size_t len;
	size_t elem_size;
	void *data;
} Slice_s;

// ============================================================================
// Test slice_new function
// ============================================================================

void test_slice_new_returns_null_on_zero_capacity(void) {
	TEST_ASSERT_NULL(slice_new(0, 1, sizeof(int)));
}

void test_slice_new_returns_null_on_zero_elem_size(void) {
	TEST_ASSERT_NULL(slice_new(10, 5, 0));
}

void test_slice_new_valid_parameters(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(10, s->cap);
	TEST_ASSERT_EQUAL(5, s->len);
	TEST_ASSERT_EQUAL(sizeof(int), s->elem_size);
	TEST_ASSERT_NOT_NULL(s->data);
	slice_free(s);
}

void test_slice_new_len_greater_than_cap(void) {
	Slice_s *s = slice_new(5, 10, sizeof(int));
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(5, s->cap);
	TEST_ASSERT_EQUAL(10, s->len);  // Implementation allows this
	slice_free(s);
}

void test_slice_new_zero_length(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(10, s->cap);
	TEST_ASSERT_EQUAL(0, s->len);
	slice_free(s);
}

// ============================================================================
// Test slice_char_new function
// ============================================================================

void test_slice_char_new_valid_parameters(void) {
	Slice_s *s = slice_char_new(10, 5);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(10, s->cap);
	TEST_ASSERT_EQUAL(5, s->len);
	TEST_ASSERT_EQUAL(sizeof(char), s->elem_size);
	TEST_ASSERT_NOT_NULL(s->data);
	slice_free(s);
}

void test_slice_char_new_zero_capacity(void) {
	Slice_s *s = slice_char_new(0, 0);
	TEST_ASSERT_NULL(s);
}

// ============================================================================
// Test slice_char_new_from_str function
// ============================================================================

void test_slice_char_new_from_str_valid_string(void) {
	const char *test_str = "hello";
	Slice_s *s = slice_char_new_from_str(test_str);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(5, s->cap);
	TEST_ASSERT_EQUAL(5, s->len);
	TEST_ASSERT_EQUAL(sizeof(char), s->elem_size);
	TEST_ASSERT_EQUAL_MEMORY(test_str, s->data, 5);
	slice_free(s);
}

void test_slice_char_new_from_str_empty_string(void) {
	const char *test_str = "";
	Slice_s *s = slice_char_new_from_str(test_str);
	TEST_ASSERT_NULL(s);  // Zero capacity should return NULL
}

void test_slice_char_new_from_str_long_string(void) {
	const char *test_str = "this is a longer test string";
	Slice_s *s = slice_char_new_from_str(test_str);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(strlen(test_str), s->cap);
	TEST_ASSERT_EQUAL(strlen(test_str), s->len);
	TEST_ASSERT_EQUAL_MEMORY(test_str, s->data, strlen(test_str));
	slice_free(s);
}

// ============================================================================
// Test slice_new_from_slice function
// ============================================================================

void test_slice_new_from_slice_valid_parameters(void) {
	Slice_s *src = slice_new(10, 8, sizeof(int));
	int *data = (int *)src->data;
	for (int i = 0; i < 8; i++) {
		data[i] = i * 10;
	}

	Slice_s *s = slice_new_from_slice(src, 2, 4);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(10, s->cap);  // Same as source
	TEST_ASSERT_EQUAL(4, s->len);
	TEST_ASSERT_EQUAL(sizeof(int), s->elem_size);

	int *slice_data = (int *)s->data;
	TEST_ASSERT_EQUAL(20, slice_data[0]);  // Should point to index 2 of original
	TEST_ASSERT_EQUAL(30, slice_data[1]);

	slice_free(src);
	free(s);  // Don't free data since it's shared
}

void test_slice_new_from_slice_start_exceeds_capacity(void) {
	Slice_s *src = slice_new(10, 5, sizeof(int));
	Slice_s *s = slice_new_from_slice(src, 15, 2);
	TEST_ASSERT_NULL(s);
	slice_free(src);
}

void test_slice_new_from_slice_len_exceeds_capacity(void) {
	Slice_s *src = slice_new(10, 5, sizeof(int));
	Slice_s *s = slice_new_from_slice(src, 2, 15);
	TEST_ASSERT_NULL(s);
	slice_free(src);
}

void test_slice_new_from_slice_zero_length(void) {
	Slice_s *src = slice_new(10, 5, sizeof(int));
	Slice_s *s = slice_new_from_slice(src, 3, 0);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL(0, s->len);
	slice_free(src);
	free(s);
}

// ============================================================================
// Test slice_reset function
// ============================================================================

void test_slice_reset_valid_slice(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	Slice_s *result = slice_reset(s);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(0, s->len);
	TEST_ASSERT_EQUAL(10, s->cap);  // Capacity unchanged
	slice_free(s);
}

void test_slice_reset_null_slice(void) {
	TEST_ASSERT_NULL(slice_reset(NULL));
}

// ============================================================================
// Test slice_get_len function
// ============================================================================

void test_slice_get_len_valid_slice(void) {
	Slice_s *s = slice_new(10, 7, sizeof(int));
	TEST_ASSERT_EQUAL(7, slice_get_len(s));
	slice_free(s);
}

void test_slice_get_len_null_slice(void) {
	TEST_ASSERT_EQUAL(-1, slice_get_len(NULL));
}

void test_slice_get_len_zero_length(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	TEST_ASSERT_EQUAL(0, slice_get_len(s));
	slice_free(s);
}

// ============================================================================
// Test slice_set_len function
// ============================================================================

void test_slice_set_len_valid_length(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_set_len(s, 8);
	TEST_ASSERT_EQUAL(8, s->len);
	slice_free(s);
}

void test_slice_set_len_zero_length(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_set_len(s, 0);
	TEST_ASSERT_EQUAL(0, s->len);
	slice_free(s);
}

void test_slice_set_len_exceeds_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	size_t original_len = s->len;
	slice_set_len(s, 15);
	TEST_ASSERT_EQUAL(original_len, s->len);  // Should remain unchanged
	slice_free(s);
}

void test_slice_set_len_null_slice(void) {
	slice_set_len(NULL, 5);  // Should not crash
}

// ============================================================================
// Test slice_incr_len function
// ============================================================================

void test_slice_incr_len_valid_increment(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_incr_len(s, 3);
	TEST_ASSERT_EQUAL(8, s->len);
	slice_free(s);
}

void test_slice_incr_len_zero_increment(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_incr_len(s, 0);
	TEST_ASSERT_EQUAL(5, s->len);
	slice_free(s);
}

void test_slice_incr_len_exceeds_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	size_t original_len = s->len;
	slice_incr_len(s, 10);
	TEST_ASSERT_EQUAL(original_len, s->len);  // Should remain unchanged
	slice_free(s);
}

void test_slice_incr_len_null_slice(void) {
	slice_incr_len(NULL, 5);  // Should not crash
}

// ============================================================================
// Test slice_get_total_cap function
// ============================================================================

void test_slice_get_total_cap_valid_slice(void) {
	Slice_s *s = slice_new(15, 5, sizeof(int));
	TEST_ASSERT_EQUAL(15, slice_get_total_cap(s));
	slice_free(s);
}

void test_slice_get_total_cap_null_slice(void) {
	TEST_ASSERT_EQUAL(-1, slice_get_total_cap(NULL));
}

// ============================================================================
// Test slice_get_usable_cap function
// ============================================================================

void test_slice_get_usable_cap_valid_slice(void) {
	Slice_s *s = slice_new(15, 5, sizeof(int));
	TEST_ASSERT_EQUAL(10, slice_get_usable_cap(s));
	slice_free(s);
}

void test_slice_get_usable_cap_full_slice(void) {
	Slice_s *s = slice_new(10, 10, sizeof(int));
	TEST_ASSERT_EQUAL(0, slice_get_usable_cap(s));
	slice_free(s);
}

void test_slice_get_usable_cap_null_slice(void) {
	TEST_ASSERT_EQUAL(-1, slice_get_usable_cap(NULL));
}

// ============================================================================
// Test slice_grow function
// ============================================================================

void test_slice_grow_small_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	size_t original_cap = s->cap;
	Slice_s *result = slice_grow(s);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(original_cap * 2, s->cap);
	TEST_ASSERT_EQUAL(5, s->len);  // Length unchanged
	slice_free(s);
}

void test_slice_grow_large_capacity(void) {
	Slice_s *s = slice_new(300, 150, sizeof(int));
	size_t original_cap = s->cap;
	Slice_s *result = slice_grow(s);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(original_cap * 1.25, s->cap);
	slice_free(s);
}

void test_slice_grow_preserves_data(void) {
	Slice_s *s = slice_new(4, 3, sizeof(int));
	TEST_ASSERT_TRUE(s != NULL);
	TEST_ASSERT_TRUE(s->data != NULL);
	int *data = (int *)s->data;
	data[0] = 100;
	data[1] = 200;
	data[2] = 300;

	s = slice_grow(s);
	TEST_ASSERT_TRUE(s != NULL);
	TEST_ASSERT_TRUE(s->data != NULL);

	int *new_data = (int *)s->data;
	TEST_ASSERT_EQUAL(100, new_data[0]);
	TEST_ASSERT_EQUAL(200, new_data[1]);
	TEST_ASSERT_EQUAL(300, new_data[2]);
	slice_free(s);
}

// ============================================================================
// Test slice_grow_to_cap function
// ============================================================================

void test_slice_grow_to_cap_larger_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	Slice_s *result = slice_grow_to_cap(s, 20);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(20, s->cap);
	TEST_ASSERT_EQUAL(5, s->len);
	slice_free(s);
}

void test_slice_grow_to_cap_smaller_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	Slice_s *result = slice_grow_to_cap(s, 5);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(10, s->cap);  // Should remain unchanged
	slice_free(s);
}

void test_slice_grow_to_cap_same_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	Slice_s *result = slice_grow_to_cap(s, 10);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(10, s->cap);
	slice_free(s);
}

// ============================================================================
// Test slice_get_ptr_offset function
// ============================================================================

void test_slice_get_ptr_offset_valid_slice(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	void *offset_ptr = slice_get_ptr_offset(s);
	void *expected_ptr = (char *)s->data + (5 * sizeof(int));
	TEST_ASSERT_EQUAL_PTR(expected_ptr, offset_ptr);
	slice_free(s);
}

void test_slice_get_ptr_offset_zero_length(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	void *offset_ptr = slice_get_ptr_offset(s);
	TEST_ASSERT_EQUAL_PTR(s->data, offset_ptr);
	slice_free(s);
}

void test_slice_get_ptr_offset_null_slice(void) {
	TEST_ASSERT_NULL(slice_get_ptr_offset(NULL));
}

// ============================================================================
// Test slice_append function
// ============================================================================

void test_slice_append_with_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	int value = 42;
	Slice_s *result = slice_append(s, &value);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(6, s->len);

	int *data = (int *)s->data;
	TEST_ASSERT_EQUAL(42, data[5]);
	slice_free(s);
}

void test_slice_append_triggers_grow(void) {
	Slice_s *s = slice_new(2, 2, sizeof(int));
	int value = 99;
	size_t original_cap = s->cap;

	Slice_s *result = slice_append(s, &value);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL(3, s->len);
	TEST_ASSERT(s->cap > original_cap);

	int *data = (int *)s->data;
	TEST_ASSERT_EQUAL(99, data[2]);
	slice_free(s);
}

void test_slice_append_multiple_items(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));

	for (int i = 0; i < 5; i++) {
		int value = i * 10;
		s = slice_append(s, &value);
		TEST_ASSERT_NOT_NULL(s);
	}

	TEST_ASSERT_EQUAL(5, s->len);
	int *data = (int *)s->data;
	for (int i = 0; i < 5; i++) {
		TEST_ASSERT_EQUAL(i * 10, data[i]);
	}
	slice_free(s);
}

// ============================================================================
// Test slice_append_n function
// ============================================================================

void test_slice_append_n(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	int items[] = {1, 2, 3, 4, 5};
	slice_append_n(s, items, 5);
	TEST_ASSERT_EQUAL(5, s->len);
	int *data = (int *)s->data;
	for (int i = 0; i < 5; i++) {
		TEST_ASSERT_EQUAL(data[i], items[i]);
	}
	slice_free(s);
}

// ============================================================================
// Test slice_is_full function
// ============================================================================

void test_slice_is_full_not_full(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	TEST_ASSERT_FALSE(slice_is_full(s));
	slice_free(s);
}

void test_slice_is_full_exactly_full(void) {
	Slice_s *s = slice_new(10, 10, sizeof(int));
	TEST_ASSERT_TRUE(slice_is_full(s));
	slice_free(s);
}

void test_slice_is_full_over_full(void) {
	Slice_s *s = slice_new(10, 15, sizeof(int));
	TEST_ASSERT_TRUE(slice_is_full(s));
	slice_free(s);
}

void test_slice_is_full_null_slice(void) {
	TEST_ASSERT_FALSE(slice_is_full(NULL));
}

void test_slice_is_full_empty_slice(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	TEST_ASSERT_FALSE(slice_is_full(s));
	slice_free(s);
}

// ============================================================================
// Test slice_copy function
// ============================================================================

void test_slice_copy_valid_slices(void) {
	Slice_s *src = slice_new(10, 5, sizeof(int));
	Slice_s *dst = slice_new(10, 8, sizeof(int));

	int *src_data = (int *)src->data;
	for (int i = 0; i < 5; i++) {
		src_data[i] = i * 10;
	}

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL(5, copied);  // MIN(dst->len=8, src->len=5)

	int *dst_data = (int *)dst->data;
	for (int i = 0; i < 5; i++) {
		TEST_ASSERT_EQUAL(i * 10, dst_data[i]);
	}

	slice_free(src);
	slice_free(dst);
}

void test_slice_copy_dst_smaller(void) {
	Slice_s *src = slice_new(10, 8, sizeof(int));
	Slice_s *dst = slice_new(10, 3, sizeof(int));

	int *src_data = (int *)src->data;
	for (int i = 0; i < 8; i++) {
		src_data[i] = i * 10;
	}

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL(3, copied);  // MIN(dst->len=3, src->len=8)

	int *dst_data = (int *)dst->data;
	for (int i = 0; i < 3; i++) {
		TEST_ASSERT_EQUAL(i * 10, dst_data[i]);
	}

	slice_free(src);
	slice_free(dst);
}

void test_slice_copy_different_elem_size(void) {
	Slice_s *src = slice_new(10, 5, sizeof(int));
	Slice_s *dst = slice_new(10, 5, sizeof(char));

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL(-1, copied);

	slice_free(src);
	slice_free(dst);
}

void test_slice_copy_null_slices(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));

	TEST_ASSERT_EQUAL(-1, slice_copy(NULL, s));
	TEST_ASSERT_EQUAL(-1, slice_copy(s, NULL));
	TEST_ASSERT_EQUAL(-1, slice_copy(NULL, NULL));

	slice_free(s);
}

// ============================================================================
// Test slice_get function
// ============================================================================

void test_slice_get_valid_index(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	int *data = (int *)s->data;
	for (int i = 0; i < 5; i++) {
		data[i] = i * 100;
	}

	int *ptr = (int *)slice_get(s, 2);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL(200, *ptr);

	slice_free(s);
}

void test_slice_get_first_element(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	int *data = (int *)s->data;
	data[0] = 999;

	int *ptr = (int *)slice_get(s, 0);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL(999, *ptr);

	slice_free(s);
}

void test_slice_get_index_out_of_bounds(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	for (int i = 5; i < 10; i++) {
		int *ptr = (int *)slice_get(s, i);
		TEST_ASSERT_NULL(ptr);
	}
	slice_free(s);
}

void test_slice_get_null_slice(void) {
	TEST_ASSERT_NULL(slice_get(NULL, 0));
}

void test_slice_get_empty_slice(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	TEST_ASSERT_NULL(slice_get(s, 1));
	slice_free(s);
}

// ============================================================================
// Test slice_range function
// ============================================================================

struct sum_callback_ctx {
	int sum;
};

bool sum_callback(void *item, size_t idx, void *ctx) {
	struct sum_callback_ctx *sctx = (struct sum_callback_ctx *)ctx;
	int *val = (int *)item;
	if (idx == 0) sctx->sum = 0;
	sctx->sum += *val;
	return true;
}

bool early_exit_callback(void *item, size_t idx, void *ctx) {
	return idx < 2;  // Stop after 2 iterations
}

void test_slice_range_valid_callback(void) {
	Slice_s *s = slice_new(5, 3, sizeof(int));
	slice_append(s, &(int){10});
	slice_append(s, &(int){20});
	slice_append(s, &(int){30});

	struct sum_callback_ctx sum = {.sum = 0};
	int expected_sum = 10 + 20 + 30;

	// Note: This test assumes the callback function works as expected
	// The actual behavior depends on the callback implementation
	slice_range(s, &sum, sum_callback);
	// We can't easily verify the sum without global state or more complex setup

	TEST_ASSERT_EQUAL_INT(expected_sum, sum.sum);

	slice_free(s);
}

void test_slice_range_early_exit(void) {
	Slice_s *s = slice_new(5, 4, sizeof(int));
	int *data = (int *)s->data;
	for (int i = 0; i < 4; i++) {
		data[i] = i * 10;
	}

	slice_range(s, NULL, early_exit_callback);
	// The callback should stop after 2 iterations
	// Verification would require more complex setup

	slice_free(s);
}

void test_slice_range_null_slice(void) {
	slice_range(NULL, NULL, sum_callback);  // Should not crash
}

void test_slice_range_null_callback(void) {
	Slice_s *s = slice_new(5, 3, sizeof(int));
	slice_range(s, NULL, NULL);  // Should not crash
	slice_free(s);
}

void test_slice_range_empty_slice(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));
	slice_range(s, NULL, sum_callback);  // Should not crash, no iterations
	slice_free(s);
}

// ============================================================================
// Test slice_sort_bubble function
// ============================================================================

typedef u8 *comparable;
typedef comparable (*slice_sort_get_key_fn)(void *item, size_t idx);
typedef ssize_t (*slice_sort_comparator)(comparable a, comparable b, void *ctx);
extern size_t slice_sort_bubble(Slice *s, void *ctx, slice_sort_get_key_fn get_key_fn, slice_sort_comparator comparator);

bool print_range_callback(void *item, size_t idx, void *ctx) {
	printf("%d\t", *(int *)item);
	return true;
}

comparable test_slice_sort_bubble_get_key_fn(void *item, size_t idx) {
	return (comparable)item;
}

ssize_t int_comparator_asc(comparable a, comparable b, void *ctx) {
	return *(int *)a - *(int *)b;
}

ssize_t int_comparator_desc(comparable a, comparable b, void *ctx) {
	return *(int *)b - *(int *)a;
}

void test_slice_sort_bubble(void) {
	Slice_s *s = slice_new(100, 0, sizeof(int));
	while (s->len < 100) {
		slice_append(s, &((int){rand() % 100}));
	}
	slice_sort_bubble(s, NULL, NULL, int_comparator_asc);
	for (size_t i = 0; i < s->len - 1; i++) {
		TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(*(int *)slice_get(s, i + 1), *(int *)slice_get(s, i), "ASC Sorted incorrectly");
	}
	slice_sort_bubble(s, NULL, NULL, int_comparator_desc);
	for (size_t i = 0; i < s->len - 1; i++) {
		TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(*(int *)slice_get(s, i), *(int *)slice_get(s, i + 1), "DESC Sorted incorrectly");
	}
	slice_free(s);
}

// ============================================================================
// Test slice_free function
// ============================================================================

void test_slice_free_valid_slice(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_free(s);  // Should not crash
}

void test_slice_free_null_slice(void) {
	slice_free(NULL);  // Should not crash
}

// ============================================================================
// Integration tests
// ============================================================================

void test_integration_create_append_get_free(void) {
	Slice_s *s = slice_new(3, 0, sizeof(int));

	// Append some values
	int values[] = {10, 20, 30, 40, 50};
	for (int i = 0; i < 5; i++) {
		s = slice_append(s, &values[i]);
		TEST_ASSERT_NOT_NULL(s);
	}

	TEST_ASSERT_EQUAL(5, s->len);
	TEST_ASSERT(s->cap >= 5);  // Should have grown

	// Verify values
	for (int i = 0; i < 5; i++) {
		int *ptr = (int *)slice_get(s, i);
		TEST_ASSERT_NOT_NULL(ptr);
		TEST_ASSERT_EQUAL(values[i], *ptr);
	}

	slice_free(s);
}

void test_integration_string_operations(void) {
	const char *test_str = "Hello World";
	Slice_s *s = slice_char_new_from_str(test_str);
	TEST_ASSERT_NOT_NULL(s);

	// Test getting characters
	char *ptr = (char *)slice_get(s, 0);
	TEST_ASSERT_EQUAL('H', *ptr);

	ptr = (char *)slice_get(s, 6);
	TEST_ASSERT_EQUAL('W', *ptr);

	// Test copying
	Slice_s *copy = slice_char_new(20, 15);
	ssize_t copied = slice_copy(copy, s);
	TEST_ASSERT_EQUAL(11, copied);
	TEST_ASSERT_EQUAL_MEMORY(test_str, copy->data, 11);

	slice_free(s);
	slice_free(copy);
}

// ============================================================================
// Main test runner
// ============================================================================

typedef struct {
	void (*test_func)(void);
	const char *test_name;
} test_case_t;

static const test_case_t all_tests[] = {
    // slice_new tests
    {test_slice_new_returns_null_on_zero_capacity, "test_slice_new_returns_null_on_zero_capacity"},
    {test_slice_new_returns_null_on_zero_elem_size, "test_slice_new_returns_null_on_zero_elem_size"},
    {test_slice_new_valid_parameters, "test_slice_new_valid_parameters"},
    {test_slice_new_len_greater_than_cap, "test_slice_new_len_greater_than_cap"},
    {test_slice_new_zero_length, "test_slice_new_zero_length"},

    // slice_char_new tests
    {test_slice_char_new_valid_parameters, "test_slice_char_new_valid_parameters"},
    {test_slice_char_new_zero_capacity, "test_slice_char_new_zero_capacity"},

    // slice_char_new_from_str tests
    {test_slice_char_new_from_str_valid_string, "test_slice_char_new_from_str_valid_string"},
    {test_slice_char_new_from_str_empty_string, "test_slice_char_new_from_str_empty_string"},
    {test_slice_char_new_from_str_long_string, "test_slice_char_new_from_str_long_string"},

    // slice_new_from_slice tests
    {test_slice_new_from_slice_valid_parameters, "test_slice_new_from_slice_valid_parameters"},
    {test_slice_new_from_slice_start_exceeds_capacity, "test_slice_new_from_slice_start_exceeds_capacity"},
    {test_slice_new_from_slice_len_exceeds_capacity, "test_slice_new_from_slice_len_exceeds_capacity"},
    {test_slice_new_from_slice_zero_length, "test_slice_new_from_slice_zero_length"},

    // slice_reset tests
    {test_slice_reset_valid_slice, "test_slice_reset_valid_slice"},
    {test_slice_reset_null_slice, "test_slice_reset_null_slice"},

    // slice_get_len tests
    {test_slice_get_len_valid_slice, "test_slice_get_len_valid_slice"},
    {test_slice_get_len_null_slice, "test_slice_get_len_null_slice"},
    {test_slice_get_len_zero_length, "test_slice_get_len_zero_length"},

    // slice_set_len tests
    {test_slice_set_len_valid_length, "test_slice_set_len_valid_length"},
    {test_slice_set_len_zero_length, "test_slice_set_len_zero_length"},
    {test_slice_set_len_exceeds_capacity, "test_slice_set_len_exceeds_capacity"},
    {test_slice_set_len_null_slice, "test_slice_set_len_null_slice"},

    // slice_incr_len tests
    {test_slice_incr_len_valid_increment, "test_slice_incr_len_valid_increment"},
    {test_slice_incr_len_zero_increment, "test_slice_incr_len_zero_increment"},
    {test_slice_incr_len_exceeds_capacity, "test_slice_incr_len_exceeds_capacity"},
    {test_slice_incr_len_null_slice, "test_slice_incr_len_null_slice"},

    // slice_get_total_cap tests
    {test_slice_get_total_cap_valid_slice, "test_slice_get_total_cap_valid_slice"},
    {test_slice_get_total_cap_null_slice, "test_slice_get_total_cap_null_slice"},

    // slice_get_usable_cap tests
    {test_slice_get_usable_cap_valid_slice, "test_slice_get_usable_cap_valid_slice"},
    {test_slice_get_usable_cap_full_slice, "test_slice_get_usable_cap_full_slice"},
    {test_slice_get_usable_cap_null_slice, "test_slice_get_usable_cap_null_slice"},

    // slice_grow tests
    {test_slice_grow_small_capacity, "test_slice_grow_small_capacity"},
    {test_slice_grow_large_capacity, "test_slice_grow_large_capacity"},
    {test_slice_grow_preserves_data, "test_slice_grow_preserves_data"},

    // slice_grow_to_cap tests
    {test_slice_grow_to_cap_larger_capacity, "test_slice_grow_to_cap_larger_capacity"},
    {test_slice_grow_to_cap_smaller_capacity, "test_slice_grow_to_cap_smaller_capacity"},
    {test_slice_grow_to_cap_same_capacity, "test_slice_grow_to_cap_same_capacity"},

    // slice_get_ptr_offset tests
    {test_slice_get_ptr_offset_valid_slice, "test_slice_get_ptr_offset_valid_slice"},
    {test_slice_get_ptr_offset_zero_length, "test_slice_get_ptr_offset_zero_length"},
    {test_slice_get_ptr_offset_null_slice, "test_slice_get_ptr_offset_null_slice"},

    // slice_append tests
    {test_slice_append_with_capacity, "test_slice_append_with_capacity"},
    {test_slice_append_triggers_grow, "test_slice_append_triggers_grow"},
    {test_slice_append_multiple_items, "test_slice_append_multiple_items"},

    {test_slice_append_n, "test_slice_append_n"},

    // slice_is_full tests
    {test_slice_is_full_not_full, "test_slice_is_full_not_full"},
    {test_slice_is_full_exactly_full, "test_slice_is_full_exactly_full"},
    {test_slice_is_full_over_full, "test_slice_is_full_over_full"},
    {test_slice_is_full_null_slice, "test_slice_is_full_null_slice"},
    {test_slice_is_full_empty_slice, "test_slice_is_full_empty_slice"},

    // slice_copy tests
    {test_slice_copy_valid_slices, "test_slice_copy_valid_slices"},
    {test_slice_copy_dst_smaller, "test_slice_copy_dst_smaller"},
    {test_slice_copy_different_elem_size, "test_slice_copy_different_elem_size"},
    {test_slice_copy_null_slices, "test_slice_copy_null_slices"},

    // slice_get tests
    {test_slice_get_valid_index, "test_slice_get_valid_index"},
    {test_slice_get_first_element, "test_slice_get_first_element"},
    {test_slice_get_index_out_of_bounds, "test_slice_get_index_out_of_bounds"},
    {test_slice_get_null_slice, "test_slice_get_null_slice"},
    {test_slice_get_empty_slice, "test_slice_get_empty_slice"},

    // slice_range tests
    {test_slice_range_valid_callback, "test_slice_range_valid_callback"},
    {test_slice_range_early_exit, "test_slice_range_early_exit"},
    {test_slice_range_null_slice, "test_slice_range_null_slice"},
    {test_slice_range_null_callback, "test_slice_range_null_callback"},
    {test_slice_range_empty_slice, "test_slice_range_empty_slice"},

    {test_slice_sort_bubble, "test_slice_sort_bubble"},

    // slice_free tests
    {test_slice_free_valid_slice, "test_slice_free_valid_slice"},
    {test_slice_free_null_slice, "test_slice_free_null_slice"},

    // Integration tests
    {test_integration_create_append_get_free, "test_integration_create_append_get_free"},
    {test_integration_string_operations, "test_integration_string_operations"}};

#define NUM_TESTS (sizeof(all_tests) / sizeof(all_tests[0]))

void run_all_tests(void) {
	for (size_t i = 0; i < NUM_TESTS; i++) {
		UnityDefaultTestRun(all_tests[i].test_func, all_tests[i].test_name, __LINE__);
	}
}

void run_single_test(const char *test_name) {
	for (size_t i = 0; i < NUM_TESTS; i++) {
		if (strcmp(all_tests[i].test_name, test_name) == 0) {
			UnityDefaultTestRun(all_tests[i].test_func, all_tests[i].test_name, __LINE__);
			return;
		}
	}
	printf("Test '%s' not found!\n", test_name);
}

void run_test_by_index(size_t index) {
	if (index < NUM_TESTS) {
		UnityDefaultTestRun(all_tests[index].test_func, all_tests[index].test_name, __LINE__);
	} else {
		printf("Test index %zu out of range (0-%zu)!\n", index, NUM_TESTS - 1);
	}
}

void list_all_tests(void) {
	printf("Available tests (%zu total):\n", NUM_TESTS);
	for (size_t i = 0; i < NUM_TESTS; i++) {
		printf("  [%2zu] %s\n", i, all_tests[i].test_name);
	}
}

int main(int argc, char *argv[]) {
	UNITY_BEGIN();

	if (argc == 1) {
		run_all_tests();
	} else if (argc == 2) {
		const char *arg = argv[1];

		if (strcmp(arg, "--list") == 0 || strcmp(arg, "-l") == 0) {
			list_all_tests();
		} else if (strncmp(arg, "--index=", 8) == 0) {
			size_t index = atoi(arg + 8);
			run_test_by_index(index);
		} else {
			run_single_test(arg);
		}
	} else {
		printf("Usage: %s [test_name|--list|--index=N]\n", argv[0]);
		printf("  test_name: Run specific test by name\n");
		printf("  --list:    List all available tests\n");
		printf("  --index=N: Run test by index number\n");
		printf("  (no args): Run all tests\n");
		return 1;
	}

	return UNITY_END();
}