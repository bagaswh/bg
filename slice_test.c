#include "slice.h"

#include <stdlib.h>
#include <string.h>

#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

// Test slice_new function
void test_slice_new_returns_null_on_invalid_input(void) {
	TEST_ASSERT_NULL(slice_new(0, 1, 1));
	TEST_ASSERT_NULL(slice_new(1, 1, 0));
}

void test_slice_new_allocates_and_initializes(void) {
	size_t cap = 4, len = 2, elem_size = sizeof(int);
	Slice *s = slice_new(cap, len, elem_size);

	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(cap, slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_size_t(len, slice_get_len(s));
	TEST_ASSERT_NOT_NULL(slice_get_ptr_offset(s));

	slice_free(s);
}

void test_slice_new_with_zero_length(void) {
	Slice *s = slice_new(10, 0, sizeof(int));
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(0, slice_get_len(s));
	TEST_ASSERT_EQUAL_size_t(10, slice_get_total_cap(s));
	slice_free(s);
}

// Test slice_char_new function
void test_slice_char_new_creates_char_slice(void) {
	Slice *s = slice_char_new(10, 5);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(10, slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_size_t(5, slice_get_len(s));
	slice_free(s);
}

// Test slice_get_len function
void test_slice_get_len_returns_negative_for_null(void) {
	TEST_ASSERT_EQUAL_INT(-1, slice_get_len(NULL));
}

void test_slice_get_len_returns_correct_length(void) {
	Slice *s = slice_new(10, 7, sizeof(int));
	TEST_ASSERT_EQUAL_INT(7, slice_get_len(s));
	slice_free(s);
}

// Test slice_set_len function
void test_slice_set_len_returns_null_for_null_slice(void) {
	TEST_ASSERT_NULL(slice_set_len(NULL, 5));
}

void test_slice_set_len_returns_null_for_invalid_len(void) {
	Slice *s = slice_new(5, 2, sizeof(int));
	TEST_ASSERT_NULL(slice_set_len(s, 10));  // len > cap
	slice_free(s);
}

void test_slice_set_len_sets_valid_length(void) {
	Slice *s = slice_new(10, 5, sizeof(int));
	void *ptr = slice_set_len(s, 8);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_size_t(8, slice_get_len(s));
	slice_free(s);
}

void test_slice_set_len_can_reduce_length(void) {
	Slice *s = slice_new(10, 8, sizeof(int));
	void *ptr = slice_set_len(s, 3);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_size_t(3, slice_get_len(s));
	slice_free(s);
}

// Test slice_incr_len function
void test_slice_incr_len_returns_null_for_null_slice(void) {
	TEST_ASSERT_NULL(slice_incr_len(NULL, 2));
}

void test_slice_incr_len_returns_null_for_overflow(void) {
	Slice *s = slice_new(5, 3, sizeof(int));
	TEST_ASSERT_NULL(slice_incr_len(s, 3));  // would exceed capacity
	slice_free(s);
}

void test_slice_incr_len_increments_length(void) {
	Slice *s = slice_new(10, 3, sizeof(int));
	void *ptr = slice_incr_len(s, 4);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_size_t(7, slice_get_len(s));
	slice_free(s);
}

void test_slice_incr_len_zero_increment(void) {
	Slice *s = slice_new(10, 5, sizeof(int));
	void *ptr = slice_incr_len(s, 0);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_size_t(5, slice_get_len(s));
	slice_free(s);
}

// Test slice_get_total_cap function
void test_slice_get_total_cap_returns_negative_for_null(void) {
	TEST_ASSERT_EQUAL_INT(-1, slice_get_total_cap(NULL));
}

void test_slice_get_total_cap_returns_capacity(void) {
	Slice *s = slice_new(15, 8, sizeof(int));
	TEST_ASSERT_EQUAL_INT(15, slice_get_total_cap(s));
	slice_free(s);
}

// Test slice_get_usable_cap function
void test_slice_get_usable_cap_returns_negative_for_null(void) {
	TEST_ASSERT_EQUAL_INT(-1, slice_get_usable_cap(NULL));
}

void test_slice_get_usable_cap_returns_remaining_capacity(void) {
	Slice *s = slice_new(10, 3, sizeof(int));
	TEST_ASSERT_EQUAL_INT(7, slice_get_usable_cap(s));
	slice_free(s);
}

void test_slice_get_usable_cap_returns_zero_when_full(void) {
	Slice *s = slice_new(5, 5, sizeof(int));
	TEST_ASSERT_EQUAL_INT(0, slice_get_usable_cap(s));
	slice_free(s);
}

// Test slice_get_ptr_offset function
void test_slice_get_ptr_offset_returns_null_for_null_slice(void) {
	TEST_ASSERT_NULL(slice_get_ptr_offset(NULL));
}

void test_slice_get_ptr_offset_returns_correct_offset(void) {
	Slice *s = slice_new(10, 3, sizeof(int));
	void *offset_ptr = slice_get_ptr_offset(s);
	TEST_ASSERT_NOT_NULL(offset_ptr);

	// Test that we can write to the offset position
	int value = 400;
	memcpy(offset_ptr, &value, sizeof(int));

	// Increment length to include the new element
	slice_incr_len(s, 1);
	TEST_ASSERT_EQUAL_size_t(4, slice_get_len(s));

	slice_free(s);
}

// Test slice_is_full function
void test_slice_is_full_returns_false_for_null(void) {
	TEST_ASSERT_FALSE(slice_is_full(NULL));
}

void test_slice_is_full_returns_true_when_full(void) {
	Slice *s = slice_new(5, 5, sizeof(int));
	TEST_ASSERT_TRUE(slice_is_full(s));
	slice_free(s);
}

void test_slice_is_full_returns_false_when_not_full(void) {
	Slice *s = slice_new(5, 3, sizeof(int));
	TEST_ASSERT_FALSE(slice_is_full(s));
	slice_free(s);
}

// Test slice_grow function
void test_slice_grow_doubles_small_capacity(void) {
	Slice *s = slice_new(4, 4, sizeof(int));
	size_t old_cap = slice_get_total_cap(s);
	s = slice_grow(s);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_TRUE(slice_get_total_cap(s) > old_cap);
	slice_free(s);
}

void test_slice_grow_increases_large_capacity(void) {
	Slice *s = slice_new(300, 300, sizeof(int));
	size_t old_cap = slice_get_total_cap(s);
	s = slice_grow(s);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_TRUE(slice_get_total_cap(s) > old_cap);
	slice_free(s);
}

// Test slice_grow_to_cap function
void test_slice_grow_to_cap_doesnt_shrink(void) {
	Slice *s = slice_new(10, 5, sizeof(int));
	size_t old_cap = slice_get_total_cap(s);
	s = slice_grow_to_cap(s, 5);  // smaller than current cap
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(old_cap, slice_get_total_cap(s));
	slice_free(s);
}

void test_slice_grow_to_cap_expands_capacity(void) {
	Slice *s = slice_new(5, 3, sizeof(int));
	s = slice_grow_to_cap(s, 15);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(15, slice_get_total_cap(s));
	TEST_ASSERT_EQUAL_size_t(3, slice_get_len(s));  // length should remain
	slice_free(s);
}

// Test slice_append function
void test_slice_append_adds_element(void) {
	Slice *s = slice_new(5, 0, sizeof(int));
	int value = 42;
	s = slice_append(s, &value);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(1, slice_get_len(s));
	slice_free(s);
}

void test_slice_append_grows_when_full(void) {
	Slice *s = slice_new(2, 0, sizeof(int));

	// Fill to capacity first
	int val1 = 1, val2 = 2;
	s = slice_append(s, &val1);
	s = slice_append(s, &val2);
	TEST_ASSERT_EQUAL_size_t(2, slice_get_len(s));
	TEST_ASSERT_TRUE(slice_is_full(s));

	// This should trigger growth
	int val3 = 3;
	s = slice_append(s, &val3);
	TEST_ASSERT_NOT_NULL(s);
	TEST_ASSERT_EQUAL_size_t(3, slice_get_len(s));
	TEST_ASSERT_TRUE(slice_get_total_cap(s) > 2);

	slice_free(s);
}

void test_slice_append_multiple_elements(void) {
	Slice *s = slice_new(10, 0, sizeof(int));

	for (int i = 0; i < 5; i++) {
		s = slice_append(s, &i);
		TEST_ASSERT_NOT_NULL(s);
	}

	TEST_ASSERT_EQUAL_size_t(5, slice_get_len(s));
	slice_free(s);
}

// Test slice_copy function
void test_slice_copy_returns_negative_for_null_params(void) {
	Slice *s = slice_new(5, 3, sizeof(int));
	TEST_ASSERT_EQUAL_INT(-1, slice_copy(NULL, s));
	TEST_ASSERT_EQUAL_INT(-1, slice_copy(s, NULL));
	TEST_ASSERT_EQUAL_INT(-1, slice_copy(NULL, NULL));
	slice_free(s);
}

void test_slice_copy_returns_negative_for_different_elem_size(void) {
	Slice *s1 = slice_new(5, 3, sizeof(int));
	Slice *s2 = slice_new(5, 3, sizeof(char));
	TEST_ASSERT_EQUAL_INT(-1, slice_copy(s1, s2));
	slice_free(s1);
	slice_free(s2);
}

void test_slice_copy_copies_data_correctly(void) {
	Slice *src = slice_new(5, 3, sizeof(int));
	Slice *dst = slice_new(5, 3, sizeof(int));

	// Fill source with test data using append
	slice_set_len(src, 0);  // Reset to empty
	int values[3] = {10, 20, 30};
	for (int i = 0; i < 3; i++) {
		src = slice_append(src, &values[i]);
	}

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL_INT(3, copied);

	slice_free(src);
	slice_free(dst);
}

void test_slice_copy_copies_min_length(void) {
	Slice *src = slice_new(5, 0, sizeof(int));
	Slice *dst = slice_new(5, 2, sizeof(int));

	// Fill source with 4 elements
	int values[4] = {1, 2, 3, 4};
	for (int i = 0; i < 4; i++) {
		src = slice_append(src, &values[i]);
	}

	ssize_t copied = slice_copy(dst, src);
	TEST_ASSERT_EQUAL_INT(2, copied);  // min(2, 4) = 2

	slice_free(src);
	slice_free(dst);
}

// Test slice_free function (basic test - hard to test memory deallocation)
void test_slice_free_handles_null(void) {
	// Should not crash
	slice_free(NULL);
	TEST_ASSERT_TRUE(true);  // If we get here, it didn't crash
}

// Integration tests
void test_slice_char_integration(void) {
	Slice *s = slice_char_new(20, 0);
	TEST_ASSERT_NOT_NULL(s);

	char c = 'A';
	for (int i = 0; i < 10; i++) {
		s = slice_append(s, &c);
		c++;
	}

	TEST_ASSERT_EQUAL_size_t(10, slice_get_len(s));
	slice_free(s);
}

void test_slice_dynamic_growth_integration(void) {
	Slice *s = slice_new(2, 0, sizeof(int));

	// Add more elements than initial capacity
	for (int i = 0; i < 10; i++) {
		s = slice_append(s, &i);
		TEST_ASSERT_NOT_NULL(s);
	}

	TEST_ASSERT_EQUAL_size_t(10, slice_get_len(s));
	TEST_ASSERT_TRUE(slice_get_total_cap(s) >= 10);

	slice_free(s);
}

int main(void) {
	UNITY_BEGIN();

	// Basic creation tests
	RUN_TEST(test_slice_new_returns_null_on_invalid_input);
	RUN_TEST(test_slice_new_allocates_and_initializes);
	RUN_TEST(test_slice_new_with_zero_length);
	RUN_TEST(test_slice_char_new_creates_char_slice);

	// Length management tests
	RUN_TEST(test_slice_get_len_returns_negative_for_null);
	RUN_TEST(test_slice_get_len_returns_correct_length);
	RUN_TEST(test_slice_set_len_returns_null_for_null_slice);
	RUN_TEST(test_slice_set_len_returns_null_for_invalid_len);
	RUN_TEST(test_slice_set_len_sets_valid_length);
	RUN_TEST(test_slice_set_len_can_reduce_length);
	RUN_TEST(test_slice_incr_len_returns_null_for_null_slice);
	RUN_TEST(test_slice_incr_len_returns_null_for_overflow);
	RUN_TEST(test_slice_incr_len_increments_length);
	RUN_TEST(test_slice_incr_len_zero_increment);

	// Capacity tests
	RUN_TEST(test_slice_get_total_cap_returns_negative_for_null);
	RUN_TEST(test_slice_get_total_cap_returns_capacity);
	RUN_TEST(test_slice_get_usable_cap_returns_negative_for_null);
	RUN_TEST(test_slice_get_usable_cap_returns_remaining_capacity);
	RUN_TEST(test_slice_get_usable_cap_returns_zero_when_full);

	// Pointer and state tests
	RUN_TEST(test_slice_get_ptr_offset_returns_null_for_null_slice);
	RUN_TEST(test_slice_get_ptr_offset_returns_correct_offset);
	RUN_TEST(test_slice_is_full_returns_false_for_null);
	RUN_TEST(test_slice_is_full_returns_true_when_full);
	RUN_TEST(test_slice_is_full_returns_false_when_not_full);

	// Growth tests
	RUN_TEST(test_slice_grow_doubles_small_capacity);
	RUN_TEST(test_slice_grow_increases_large_capacity);
	RUN_TEST(test_slice_grow_to_cap_doesnt_shrink);
	RUN_TEST(test_slice_grow_to_cap_expands_capacity);

	// Append tests
	RUN_TEST(test_slice_append_adds_element);
	RUN_TEST(test_slice_append_grows_when_full);
	RUN_TEST(test_slice_append_multiple_elements);

	// Copy tests
	RUN_TEST(test_slice_copy_returns_negative_for_null_params);
	RUN_TEST(test_slice_copy_returns_negative_for_different_elem_size);
	RUN_TEST(test_slice_copy_copies_data_correctly);
	RUN_TEST(test_slice_copy_copies_min_length);

	// Memory management tests
	RUN_TEST(test_slice_free_handles_null);

	// Integration tests
	RUN_TEST(test_slice_char_integration);
	RUN_TEST(test_slice_dynamic_growth_integration);

	return UNITY_END();
}