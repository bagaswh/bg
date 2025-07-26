#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
digits_count(int n)
{
    int digits = 0;
    while (n) {
        n /= 10;
        digits++;
    }
    return digits;
}

// ANSI color codes
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_GREEN "\033[32m"
#define COLOR_PURPLE "\033[35m"
#define COLOR_RESET "\033[0m"

#define BG_LIGHT_ORANGE "\033[48;5;215m"
#define BG_LIGHT_BLUE "\033[48;5;117m"
#define BG_YELLOW "\033[43m"

void
print_arr_state(int *arr, ssize_t i, ssize_t j, ssize_t p, ssize_t r,
                ssize_t pivot)
{
    printf("i=%ld j=%ld p=%ld pivot=%ld\n", i, j, p, pivot);

    for (size_t ii = 0; ii < r; ii++) {
        int has_space = 0;
        int spaces = 0;
        if (ii == i) {
            spaces++;
        }
        if (ii == j) {
            spaces++;
        }
        if (ii == p)
            spaces++;
        // if ((ii == j || ii == i + 1) && arr[j] < pivot)
        //     printf(COLOR_YELLOW);
        // if (ii >= p && ii <= i)
        //     printf(BG_LIGHT_ORANGE);
        // else if (ii >= i && ii <= j - 1 && arr[ii] != pivot)
        //     printf(BG_LIGHT_BLUE);
        // else if (arr[ii] == pivot)
        //     printf(BG_YELLOW);
        printf("%d", arr[ii]);
        // printf(COLOR_RESET);
        if (!spaces)
            printf(" ");
        else {
            do {
                printf(" ");
                spaces--;
            } while (spaces > 0);
        }
    }

    printf("\n");

    for (size_t ii = 0; ii < r; ii++) {
        int has_arrow_ptr = 0;
        int spaces = 0;
        if (ii == i) {
            has_arrow_ptr = 1;
            spaces++;
        }
        if (ii == j) {
            has_arrow_ptr = 1;
            spaces++;
        }
        if (ii == p) {
            has_arrow_ptr = 1;
            spaces++;
        }
        int digsc = digits_count(arr[ii]);
        spaces = abs(spaces - digsc);
        if (has_arrow_ptr) {
            // Color the arrow based on what pointer it represents
            if (ii == p)
                printf(COLOR_RED);
            else if (ii == i)
                printf(COLOR_BLUE);
            else if (ii == j)
                printf(COLOR_GREEN);
            else if (arr[ii] == pivot)
                printf(COLOR_PURPLE);

            printf("^");
            printf(COLOR_RESET);
            spaces++;
            do {
                printf(" ");
                spaces--;
            } while (spaces > 0);
        } else {
            printf("  ");
        }
    }

    printf("\n");

    for (size_t ii = 0; ii < r; ii++) {
        int has_arrow_ptr = 0;
        int has_arrow_ptr_for_i = 0;
        int has_arrow_ptr_for_j = 0;
        int has_arrow_ptr_for_p = 0;
        int has_arrow_ptr_for_pivot = 0;
        if (ii == i) {
            has_arrow_ptr = 1;
            has_arrow_ptr_for_i = 1;
        }
        if (ii == j) {
            has_arrow_ptr = 1;
            has_arrow_ptr_for_j = 1;
        }
        if (ii == p) {
            has_arrow_ptr = 1;
            has_arrow_ptr_for_p = 1;
        }
        if (has_arrow_ptr) {
            if (has_arrow_ptr_for_i)
                printf(COLOR_BLUE "i" COLOR_RESET);
            if (has_arrow_ptr_for_j)
                printf(COLOR_GREEN "j" COLOR_RESET);
            if (has_arrow_ptr_for_p)
                printf(COLOR_RED "p" COLOR_RESET);
            if (has_arrow_ptr_for_pivot)
                printf(COLOR_PURPLE "r" COLOR_RESET);
            printf(" ");
        } else {
            int digsc = digits_count(arr[ii]);
            printf(" ");
            do {
                printf(" ");
                digsc--;
            } while (digsc > 0);
        }
    }

    printf("\n");
}

void
swap_ints(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void
partition(int *items, size_t p, size_t r)
{
    int *pivot = &items[r - 1];
    ssize_t i = p - 1;
    ssize_t j = p;
    while (j < r - 2) {
        printf("\n");
        print_arr_state(items, i, j, p, r, *pivot);
        printf("\n");
        if (items[j] <= *pivot) {
            i++;
            printf("%d at index %ld is lt pivot (%d); swapping this with %d "
                   "at index "
                   "%ld\n",
                   items[j], j, *pivot, items[i], i);
            swap_ints(&items[i], &items[j]);
        }
        j++;
    }
    swap_ints(&items[i + 1], pivot);
    pivot = &items[i + 1];
    print_arr_state(items, i, r, p, r, *pivot);
}

void
hoare_partition(int *arr, size_t p, size_t r)
{
    int *pivot = &arr[p];
    ssize_t i = p;
    ssize_t j = r;
    while (1) {
        while (1) {
            j--;
            print_arr_state(arr, i, j, p, r, *pivot);
            if (arr[j] <= *pivot) {
                break;
            }
        }
        while (1) {
            i++;
            print_arr_state(arr, i, j, p, r, *pivot);
            if (arr[i] >= *pivot) {
                break;
            }
        }
        if (i < j) {
            int swap_pivot = 0;
            if (arr[j] == *pivot) {
                swap_pivot = 1;
            }
            swap_ints(&arr[i], &arr[j]);
            if (swap_pivot) {
                *pivot = arr[i];
            }
        } else {
            break;
        }
        print_arr_state(arr, i, j, p, r, *pivot);
    }
    print_arr_state(arr, i, j, p, r, *pivot);
}

size_t
sort_insertion(int *arr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ssize_t j = i;
        while ((j - 1) >= 0 && arr[j] < arr[j - 1]) {
            swap_ints(&arr[j - 1], &arr[j]);
            j--;
        }
    }
}

int
mo3_pick_median(int *arr, int *pivot_picker_buf, struct rand *rng, size_t n)
{
    int one_idx, two_idx, three_idx;
    one_idx = fast_rand_n(rng, n);
    pivot_picker_buf[0] = arr[one_idx];
    do {
        two_idx = fast_rand_n(rng, n);
        pivot_picker_buf[1] = arr[two_idx];
    } while (pivot_picker_buf[0] == pivot_picker_buf[1]);
    do {
        three_idx = fast_rand_n(rng, n);
        pivot_picker_buf[2] = arr[three_idx];
    } while (pivot_picker_buf[0] == pivot_picker_buf[2]
             || pivot_picker_buf[1] == pivot_picker_buf[2]);
    sort_insertion(pivot_picker_buf, 3);
    printf("0=%d 1=%d 2=%d\n", pivot_picker_buf[0], pivot_picker_buf[1],
           pivot_picker_buf[2]);
    if (arr[one_idx] == pivot_picker_buf[1])
        return one_idx;
    if (arr[two_idx] == pivot_picker_buf[1])
        return two_idx;
    if (arr[three_idx] == pivot_picker_buf[1])
        return three_idx;
}

int
mo3_partition(int *arr, int *pivot_picker_buf, struct rand *rng, size_t p,
              size_t r)
{
    int median_idx = mo3_pick_median(arr, pivot_picker_buf, rng, r);
    swap_ints(&arr[median_idx], &arr[r - 1]);

    int *pivot = &arr[r - 1];
    ssize_t i = p;
    ssize_t j = r - 2;
    while (1) {
        print_arr_state(arr, i, r, p, r, *pivot);
        while (1) {
            print_arr_state(arr, i, r, p, r, *pivot);
            if (arr[i] >= *pivot) {
                break;
            }
            i++;
        }
        while (1) {
            print_arr_state(arr, i, r, p, r, *pivot);
            if (arr[j] <= *pivot) {
                break;
            }
            j--;
        }
        if (i > j) {
            break;
        }
        swap_ints(&arr[i], &arr[j]);
    }
    swap_ints(&arr[i], &arr[r - 1]);
    pivot = &arr[i];
    print_arr_state(arr, i, r, p, r, *pivot);
    return i;
}

#define arrlen(arr, typ) (sizeof(arr) / sizeof(typ))

void
bg_qsort(int *arr, int *pivot_picker_buf, struct rand *rng, size_t p,
         size_t r)
{
    if (r - p <= 10) {
        sort_insertion(arr, r);
        return;
    }
    int pivot_idx = mo3_partition(arr, pivot_picker_buf, &rng, p, r);
    bg_qsort(arr, pivot_picker_buf, rng, 0, pivot_idx - 1);
    bg_qsort(arr, pivot_picker_buf, rng, pivot_idx + 1, r);
}

int
main(void)
{
    struct rand rng = init_rand();
    int arr[] = { 2, 8, 7, 1, 3, 5, 6, 4 };
    int *pivot_picker_buf[3];
    bg_qsort(&arr[0], &pivot_picker_buf[0], &rng, 0, arrlen(arr, int));
    print_arr_state(arr, 0, 0, 0, arrlen(arr, int), 0);
}