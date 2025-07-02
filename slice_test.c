#include "slice.h"

#include <stdlib.h>
#include <string.h>

#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Test slice_new function
// ============================================================================

typedef struct Slice_s {
	size_t cap;
	size_t len;
	size_t elem_size;
	void *data;
} Slice_s;

void test_slice_new_returns_null_on_zero_capacity(void) {
	TEST_ASSERT_NULL(slice_new(0, 1, sizeof(int)));
}

void test_slice_new_returns_null_on_zero_elem_size(void) {
	TEST_ASSERT_NULL(slice_new(10, 5, 0));
}

void test_slice_new_returns_null_on_both_zero(void) {
	TEST_ASSERT_NULL(slice_new(0, 0, 0));
}

void test_slice_new_allocates_and_initializes_correctly(void) {
	size_t cap = 10, len = 5, elem_size = sizeof(int);
	Slice_s *s = slice_new(cap, len, elem_size);

	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(cap, slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_size_t(len, slice_get_len(s));
	TEST_ASSERT_NOT_NULL(slice_get_ptr_offset(s));

	slice_free(s);
}

void test_slice_new_with_len_greater_than_cap(void) {
	Slice_s *s = slice_new(5, 10, sizeof(int));
	TEST_ASSERT_NOT_NULL(s);  // Should still create slice
	TEST_ASSERT_EQUAL_size_t(5, slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_size_t(10, slice_get_len(s));
	slice_free(s);
}

void test_slice_new_with_different_element_sizes(void) {
	Slice_s *s1 = slice_new(10, 5, sizeof(char));
	Slice_s *s2 = slice_new(10, 5, sizeof(double));
	Slice_s *s3 = slice_new(10, 5, sizeof(void *));

	TEST_ASSERT_NOT_NULL(s1);
	TEST_ASSERT_NOT_NULL(s2);
	TEST_ASSERT_NOT_NULL(s3);

	slice_free(s1);
	slice_free(s2);
	slice_free(s3);
}

// ============================================================================
// Test slice_char_new and slice_char_new_from_str functions
// ============================================================================

void test_slice_char_new_creates_char_slice(void) {
	Slice_s *s = slice_char_new(20, 10);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(20, slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_size_t(10, slice_get_len(s));
	slice_free(s);
}

void test_slice_char_new_from_str_with_valid_string(void) {
	const char *test_str = "Hello, World!";
	Slice_s *s = slice_char_new_from_str(test_str);

	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(strlen(test_str), slice_get_len(s));
	TEST_ASSERT_EQUAL_size_t(strlen(test_str), slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_STRING_LEN(test_str, (char *)s->data, strlen(test_str));

	slice_free(s);
}

void test_slice_char_new_from_str_with_empty_string(void) {
	const char *test_str = "";
	Slice_s *s = slice_char_new_from_str(test_str);

	TEST_ASSERT_NULL(s);

	slice_free(s);
}

void test_slice_char_new_from_str_with_long_string(void) {
	char long_str[1000];
	memset(long_str, 'A', 999);
	long_str[999] = '\0';

	Slice_s *s = slice_char_new_from_str(long_str);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(999, slice_get_len(s));
	slice_free(s);
}

// ============================================================================
// Test slice_new_from_slice function
// ============================================================================

void test_slice_new_from_slice_valid_parameters(void) {
	Slice_s *original = slice_new(10, 8, sizeof(int));
	Slice_s *sub = slice_new_from_slice(original, 2, 4);

	TEST_ASSERT_NOT_NULL(sub);
	TEST_ASSERT_EQUAL_size_t(4, slice_get_len(sub));
	TEST_ASSERT_EQUAL_size_t(10, slice_get_total_cap(sub));
	TEST_ASSERT_EQUAL_PTR((char *)original->data + 2 * sizeof(int), sub->data);

	slice_free(original);
	free(sub);  // Don't free data since it belongs to original
}

void test_slice_new_from_slice_start_at_boundary(void) {
	Slice_s *original = slice_new(10, 5, sizeof(int));
	Slice_s *sub = slice_new_from_slice(original, 9, 1);

	TEST_ASSERT_NOT_NULL(sub);
	TEST_ASSERT_EQUAL_size_t(1, slice_get_len(sub));

	slice_free(original);
	free(sub);
}

void test_slice_new_from_slice_invalid_start(void) {
	Slice_s *original = slice_new(10, 5, sizeof(int));
	Slice_s *sub = slice_new_from_slice(original, 10, 1);  // start >= cap

	TEST_ASSERT_NULL(sub);

	slice_free(original);
}

void test_slice_new_from_slice_invalid_length(void) {
	Slice_s *original = slice_new(10, 5, sizeof(int));
	Slice_s *sub = slice_new_from_slice(original, 0, 11);  // len > cap

	TEST_ASSERT_NULL(sub);

	slice_free(original);
}

// ============================================================================
// Test slice length and capacity functions
// ============================================================================

void test_slice_get_len_with_null(void) {
	TEST_ASSERT_EQUAL_INT(-1, slice_get_len(NULL));
}

void test_slice_get_len_with_valid_slice(void) {
	Slice_s *s = slice_new(10, 7, sizeof(int));
	TEST_ASSERT_EQUAL_INT(7, slice_get_len(s));
	slice_free(s);
}

void test_slice_set_len_with_null(void) {
	// TEST_ASSERT_NULL(slice_set_len(NULL, 5));
}

void test_slice_set_len_with_valid_length(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_set_len(s, 8);

	// TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_size_t(8, slice_get_len(s));
	// TEST_ASSERT_EQUAL_PTR(s->data, result);

	slice_free(s);
}

void test_slice_set_len_exceeds_capacity(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_set_len(s, 15);

	// TEST_ASSERT_NULL(result);
	TEST_ASSERT_EQUAL_size_t(5, slice_get_len(s));  // Should remain unchanged

	slice_free(s);
}

void test_slice_incr_len_with_null(void) {
	slice_incr_len(NULL, 2);
}

void test_slice_incr_len_valid_increment(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	slice_incr_len(s, 3);

	TEST_ASSERT_EQUAL_size_t(8, slice_get_len(s));

	slice_free(s);
}

void test_slice_incr_len_exceeds_capacity(void) {
	Slice_s *s = slice_new(10, 8, sizeof(int));
	slice_incr_len(s, 5);

	// TEST_ASSERT_NULL(result);
	TEST_ASSERT_EQUAL_size_t(8, slice_get_len(s));  // Should remain unchanged

	slice_free(s);
}

void test_slice_get_total_cap_with_null(void) {
	TEST_ASSERT_EQUAL_INT(-1, slice_get_total_cap(NULL));
}

void test_slice_get_total_cap_with_valid_slice(void) {
	Slice_s *s = slice_new(15, 10, sizeof(int));
	TEST_ASSERT_EQUAL_INT(15, slice_get_total_cap(s));
	slice_free(s);
}

void test_slice_get_usable_cap_with_null(void) {
	TEST_ASSERT_EQUAL_INT(-1, slice_get_usable_cap(NULL));
}

void test_slice_get_usable_cap_with_valid_slice(void) {
	Slice_s *s = slice_new(15, 8, sizeof(int));
	TEST_ASSERT_EQUAL_INT(7, slice_get_usable_cap(s));
	slice_free(s);
}

// ============================================================================
// Test slice_reset function
// ============================================================================

void test_slice_reset_with_null(void) {
	TEST_ASSERT_NULL(slice_reset(NULL));
}

void test_slice_reset_with_valid_slice(void) {
	Slice_s *s = slice_new(10, 8, sizeof(int));
	Slice_s *result = slice_reset(s);

	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL_size_t(0, slice_get_len(s));
	TEST_ASSERT_EQUAL_size_t(10, slice_get_total_cap(s));

	slice_free(s);
}

// ============================================================================
// Test slice_grow and slice_grow_to_cap functions
// ============================================================================

void test_slice_grow_small_capacity(void) {
	Slice_s *s = slice_new(100, 50, sizeof(int));
	size_t original_cap = slice_get_total_cap(s);

	Slice_s *result = slice_grow(s);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL_size_t(original_cap * 2, slice_get_total_cap(s));

	slice_free(s);
}

void test_slice_grow_large_capacity(void) {
	Slice_s *s = slice_new(300, 150, sizeof(int));
	size_t original_cap = slice_get_total_cap(s);

	Slice_s *result = slice_grow(s);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_size_t(original_cap * 1.25, slice_get_total_cap(s));

	slice_free(s);
}

void test_slice_grow_to_cap_smaller_cap(void) {
	Slice_s *s = slice_new(20, 10, sizeof(int));
	Slice_s *result = slice_grow_to_cap(s, 15);

	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL_size_t(20, slice_get_total_cap(s));  // Should remain unchanged

	slice_free(s);
}

void test_slice_grow_to_cap_larger_cap(void) {
	Slice_s *s = slice_new(20, 10, sizeof(int));
	Slice_s *result = slice_grow_to_cap(s, 30);

	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_size_t(30, slice_get_total_cap(s));

	slice_free(s);
}

// ============================================================================
// Test slice_append function
// ============================================================================

bool range_cb(void *item, size_t idx, size_t len, size_t cap) {
	printf("%ld: %d\n", idx, *(int *)item);
}

void test_slice_append_with_available_space(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	int value = 42;

	Slice_s *result = slice_append(s, &value);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(s, result);
	TEST_ASSERT_EQUAL_size_t(6, slice_get_len(s));

	slice_range(s, range_cb);

	int *data = (int *)slice_get(s, 5);
	TEST_ASSERT_EQUAL_INT(42, *data);

	slice_free(s);
}

void test_slice_append_when_full_grows_slice(void) {
	Slice_s *s = slice_new(5, 5, sizeof(int));
	int value = 99;

	Slice_s *result = slice_append(s, &value);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_size_t(6, slice_get_len(s));
	TEST_ASSERT_TRUE(slice_get_total_cap(s) > 5);  // Should have grown

	slice_free(s);
}

void test_slice_append_multiple_items(void) {
	Slice_s *s = slice_new(10, 0, sizeof(int));

	for (int i = 0; i < 5; i++) {
		s = slice_append(s, &i);
		TEST_ASSERT_NOT_NULL(s);
	}

	TEST_ASSERT_EQUAL_size_t(5, slice_get_len(s));
	int *data = (int *)s->data;
	for (int i = 0; i < 5; i++) {
		TEST_ASSERT_EQUAL_INT(i, data[i]);
	}

	slice_free(s);
}

// ============================================================================
// Test slice_is_full function
// ============================================================================

void test_slice_is_full_with_null(void) {
	TEST_ASSERT_FALSE(slice_is_full(NULL));
}

void test_slice_is_full_when_not_full(void) {
	Slice_s *s = slice_new(10, 7, sizeof(int));
	TEST_ASSERT_FALSE(slice_is_full(s));
	slice_free(s);
}

void test_slice_is_full_when_full(void) {
	Slice_s *s = slice_new(10, 10, sizeof(int));
	TEST_ASSERT_TRUE(slice_is_full(s));
	slice_free(s);
}

void test_slice_is_full_when_overfull(void) {
	Slice_s *s = slice_new(10, 15, sizeof(int));
	TEST_ASSERT_TRUE(slice_is_full(s));
	slice_free(s);
}

// ============================================================================
// Test slice_copy function
// ============================================================================

void test_slice_copy_with_null_dst(void) {
	Slice_s *src = slice_new(5, 3, sizeof(int));
	TEST_ASSERT_EQUAL_INT(-1, slice_copy(NULL, src));
	slice_free(src);
}

void test_slice_copy_with_null_src(void) {
	Slice_s *dst = slice_new(5, 3, sizeof(int));
	TEST_ASSERT_EQUAL_INT(-1, slice_copy(dst, NULL));
	slice_free(dst);
}

void test_slice_copy_different_elem_sizes(void) {
	Slice_s *dst = slice_new(5, 3, sizeof(int));
	Slice_s *src = slice_new(5, 3, sizeof(char));

	TEST_ASSERT_EQUAL_INT(-1, slice_copy(dst, src));

	slice_free(dst);
	slice_free(src);
}

void test_slice_copy_successful_copy(void) {
	Slice_s *src = slice_new(5, 3, sizeof(int));
	Slice_s *dst = slice_new(5, 3, sizeof(int));

	// Fill source with test data
	int *src_data = (int *)src->data;
	src_data[0] = 10;
	src_data[1] = 20;
	src_data[2] = 30;

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL_INT(3, copied);

	int *dst_data = (int *)dst->data;
	TEST_ASSERT_EQUAL_INT(10, dst_data[0]);
	TEST_ASSERT_EQUAL_INT(20, dst_data[1]);
	TEST_ASSERT_EQUAL_INT(30, dst_data[2]);

	slice_free(src);
	slice_free(dst);
}

void test_slice_copy_partial_copy_dst_smaller(void) {
	Slice_s *src = slice_new(5, 4, sizeof(int));
	Slice_s *dst = slice_new(5, 2, sizeof(int));

	// Fill source with test data
	int *src_data = (int *)src->data;
	for (int i = 0; i < 4; i++) {
		src_data[i] = i * 10;
	}

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL_INT(2, copied);  // Only 2 should be copied

	int *dst_data = (int *)dst->data;
	TEST_ASSERT_EQUAL_INT(0, dst_data[0]);
	TEST_ASSERT_EQUAL_INT(10, dst_data[1]);

	slice_free(src);
	slice_free(dst);
}

// ============================================================================
// Test slice_get function
// ============================================================================

void test_slice_get_with_null(void) {
	TEST_ASSERT_NULL(slice_get(NULL, 0));
}

void test_slice_get_valid_index(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	int *data = (int *)s->data;
	data[3] = 42;

	void *result = slice_get(s, 3);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(42, *(int *)result);

	slice_free(s);
}

void test_slice_get_index_out_of_bounds(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	TEST_ASSERT_NULL(slice_get(s, 6));  // Index > len
	slice_free(s);
}

void test_slice_get_index_at_boundary(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	int *data = (int *)s->data;
	data[4] = 99;

	void *result = slice_get(s, 4);  // Last valid index
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(99, *(int *)result);

	slice_free(s);
}

// ============================================================================
// Test slice_get_ptr_offset function
// ============================================================================

void test_slice_get_ptr_offset_with_null(void) {
	TEST_ASSERT_NULL(slice_get_ptr_offset(NULL));
}

void test_slice_get_ptr_offset_valid_slice(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	void *offset_ptr = slice_get_ptr_offset(s);

	TEST_ASSERT_NOT_NULL(offset_ptr);
	TEST_ASSERT_EQUAL_PTR((char *)s->data + 5 * sizeof(int), offset_ptr);

	slice_free(s);
}

// ============================================================================
// Test slice_free function
// ============================================================================

void test_slice_free_with_null(void) {
	// Should not crash
	slice_free(NULL);
	TEST_PASS();
}

void test_slice_free_with_valid_slice(void) {
	Slice_s *s = slice_new(10, 5, sizeof(int));
	// Should not crash
	slice_free(s);
	TEST_PASS();
}

// ============================================================================
// Integration and edge case tests
// ============================================================================

void test_slice_lifecycle_complete(void) {
	// Create slice
	Slice_s *s = slice_new(5, 0, sizeof(int));
	TEST_ASSERT_NOT_NULL(s);

	// Add some data
	for (int i = 0; i < 5; i++) {
		s = slice_append(s, &i);
		TEST_ASSERT_NOT_NULL(s);
	}

	// Should be full now
	TEST_ASSERT_TRUE(slice_is_full(s));

	// Add one more (should grow)
	int extra = 99;
	s = slice_append(s, &extra);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_FALSE(slice_is_full(s));

	// Reset and verify
	s = slice_reset(s);
	TEST_ASSERT_EQUAL_size_t(0, slice_get_len(s));

	slice_free(s);
}

void test_slice_with_large_elements(void) {
	struct large_struct {
		char data[1000];
		int id;
	};

	Slice_s *s = slice_new(10, 0, sizeof(struct large_struct));
	TEST_ASSERT_NOT_NULL(s);

	struct large_struct item = {.id = 42};
	memset(item.data, 'A', 999);
	item.data[999] = '\0';

	s = slice_append(s, &item);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(1, slice_get_len(s));

	struct large_struct *retrieved = (struct large_struct *)slice_get(s, 0);
	TEST_ASSERT_NOT_NULL(retrieved);
	TEST_ASSERT_EQUAL_INT(42, retrieved->id);

	slice_free(s);
}

// ============================================================================
// Main test runner
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// slice_new tests
	RUN_TEST(test_slice_new_returns_null_on_zero_capacity);
	RUN_TEST(test_slice_new_returns_null_on_zero_elem_size);
	RUN_TEST(test_slice_new_returns_null_on_both_zero);
	RUN_TEST(test_slice_new_allocates_and_initializes_correctly);
	RUN_TEST(test_slice_new_with_len_greater_than_cap);
	RUN_TEST(test_slice_new_with_different_element_sizes);

	// slice_char_new tests
	RUN_TEST(test_slice_char_new_creates_char_slice);
	RUN_TEST(test_slice_char_new_from_str_with_valid_string);
	RUN_TEST(test_slice_char_new_from_str_with_empty_string);
	RUN_TEST(test_slice_char_new_from_str_with_long_string);

	// slice_new_from_slice tests
	RUN_TEST(test_slice_new_from_slice_valid_parameters);
	RUN_TEST(test_slice_new_from_slice_start_at_boundary);
	RUN_TEST(test_slice_new_from_slice_invalid_start);
	RUN_TEST(test_slice_new_from_slice_invalid_length);

	// Length and capacity tests
	RUN_TEST(test_slice_get_len_with_null);
	RUN_TEST(test_slice_get_len_with_valid_slice);
	RUN_TEST(test_slice_set_len_with_null);
	RUN_TEST(test_slice_set_len_with_valid_length);
	RUN_TEST(test_slice_set_len_exceeds_capacity);
	RUN_TEST(test_slice_incr_len_with_null);
	RUN_TEST(test_slice_incr_len_valid_increment);
	RUN_TEST(test_slice_incr_len_exceeds_capacity);
	RUN_TEST(test_slice_get_total_cap_with_null);
	RUN_TEST(test_slice_get_total_cap_with_valid_slice);
	RUN_TEST(test_slice_get_usable_cap_with_null);
	RUN_TEST(test_slice_get_usable_cap_with_valid_slice);

	// slice_reset tests
	RUN_TEST(test_slice_reset_with_null);
	RUN_TEST(test_slice_reset_with_valid_slice);

	// Growth tests
	RUN_TEST(test_slice_grow_small_capacity);
	RUN_TEST(test_slice_grow_large_capacity);
	RUN_TEST(test_slice_grow_to_cap_smaller_cap);
	RUN_TEST(test_slice_grow_to_cap_larger_cap);

	// Append tests
	RUN_TEST(test_slice_append_with_available_space);
	RUN_TEST(test_slice_append_when_full_grows_slice);
	RUN_TEST(test_slice_append_multiple_items);

	// is_full tests
	RUN_TEST(test_slice_is_full_with_null);
	RUN_TEST(test_slice_is_full_when_not_full);
	RUN_TEST(test_slice_is_full_when_full);
	RUN_TEST(test_slice_is_full_when_overfull);

	// Copy tests
	RUN_TEST(test_slice_copy_with_null_dst);
	RUN_TEST(test_slice_copy_with_null_src);
	RUN_TEST(test_slice_copy_different_elem_sizes);
	RUN_TEST(test_slice_copy_successful_copy);
	RUN_TEST(test_slice_copy_partial_copy_dst_smaller);

	// Get tests
	RUN_TEST(test_slice_get_with_null);
	RUN_TEST(test_slice_get_valid_index);
	RUN_TEST(test_slice_get_index_out_of_bounds);
	RUN_TEST(test_slice_get_index_at_boundary);

	// Pointer offset tests
	RUN_TEST(test_slice_get_ptr_offset_with_null);
	RUN_TEST(test_slice_get_ptr_offset_valid_slice);

	// Free tests
	RUN_TEST(test_slice_free_with_null);
	RUN_TEST(test_slice_free_with_valid_slice);

	// Integration tests
	RUN_TEST(test_slice_lifecycle_complete);
	RUN_TEST(test_slice_with_large_elements);

	return UNITY_END();
}