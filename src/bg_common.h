#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef bg_likely
#    define bg_likely(x) __builtin_expect(!!(x), 1)
#    define bg_unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifndef BG_INLINE
#    define BG_INLINE __attribute__((always_inline))
#    define BG_NOINLINE __attribute__((noinline))
#endif

static const bool print_stacktrace_before_abort =
#ifdef BG_NO_PRINT_STACK_TRACE_BEFORE_ABORT
    false
#else
    true
#endif
    ;

static const bool assert_enabled =
#ifdef NDEBUG
    false
#else
    true
#endif
    ;

// Copied from
// https://github.com/gingerBill/gb/
#ifndef BG_ENDIAN_ORDER
#    define BG_ENDIAN_ORDER
#    define BG_IS_BIG_ENDIAN (!*(u8 *) &(u16) { 1 })
#    define BG_IS_LITTLE_ENDIAN (!BG_IS_BIG_ENDIAN)
#endif

// Copied from
// https://github.com/gingerBill/gb/
#ifndef BG_DEBUG_TRAP
#    if defined(_MSC_VER)
#        if _MSC_VER >= 1300
#            define BG_DEBUG_TRAP() __asm int 3 /* Trap to debugger! */
#        else
#            define BG_DEBUG_TRAP() __debugbreak()
#        endif
#    elif defined(__clang__)
#        define BG_DEBUG_TRAP() __builtin_debugtrap()
#    else
#        define BG_DEBUG_TRAP() __asm int 3
#    endif
#endif

#ifdef __BG_RUNNING_TEST__
#    include <setjmp.h>
static void
__bg_abort__(void)
{
    // see testutils.h for context
    extern jmp_buf test_resume_env;
    longjmp(test_resume_env, 1);
}
#else
static void
__bg_abort__(void)
{
    BG_DEBUG_TRAP();
}
#endif

#ifndef BG_NO_PRINT_STACK_TRACE
#    include <execinfo.h>
static void
print_backtrace(char *component)
{
    void *bt[32];
    int bt_size = backtrace(bt, 32);
    char **bt_syms = backtrace_symbols(bt, bt_size);
    if (bt_syms) {
        printf("<%s>: Stack trace:\n", component);
        for (int i = 0; i < bt_size; ++i) {
            printf("  [%d] %s\n", i, bt_syms[i]);
        }
        printf("\n");
        free(bt_syms);
    } else {
        printf("<%s>: Failed to get stack trace symbols\n", component);
    }
}
#else
static void BG_INLINE
print_backtrace(char *component)
{
}
#endif

#ifndef bg_assert
#    define bg_assert(component, condition, fmt, ...)                  \
        do {                                                           \
            if (bg_unlikely(!(condition) && assert_enabled)) {         \
                fprintf(stderr,                                        \
                        "ASSERT: <%s>: at func %s(): %s:%d " fmt "\n", \
                        component, __func__, __FILE__,                 \
                        __LINE__ __VA_OPT__(, ) __VA_ARGS__);          \
                if (print_stacktrace_before_abort) {                   \
                    print_backtrace(component);                        \
                }                                                      \
                __bg_abort__();                                        \
            }                                                          \
        } while (0)
#endif

#ifndef bg_arr_length
#    define bg_arr_length(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#ifndef bg_swap
#    define bg_swap(Type, a, b) \
        do {                    \
            Type tmp = (a);     \
            (a) = (b);          \
            (b) = tmp;          \
        } while (0)
#endif

static void
bg_swap_generic(void *a, void *b, void *tmp, size_t size)
{
    memcpy(tmp, a, size);
    memcpy(a, b, size);
    memcpy(b, tmp, size);
}

#endif // COMMON_H