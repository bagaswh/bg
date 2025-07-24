#ifndef COMMON_H
#define COMMON_H

#include <execinfo.h> // for backtrace_symbols

#define bg_likely(x) __builtin_expect(!!(x), 1)
#define bg_unlikely(x) __builtin_expect(!!(x), 0)

#define BG_INLINE __attribute__((always_inline))
#define BG_NOINLINE __attribute__((noinline))

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

#ifdef __BG_RUNNING_TEST__
#include <setjmp.h>
#define __bg_abort__                    \
    do {                                \
        extern jmp_buf test_resume_env; \
        longjmp(test_resume_env, 1);    \
    } while (0)
#else
#define __bg_abort__ abort()
#endif

#define print_backtrace(component)                                        \
    do {                                                                  \
        if (bg_unlikely(print_stacktrace_before_abort)) {                 \
            void *bt[32];                                                 \
            int bt_size = backtrace(bt, 32);                              \
            char **bt_syms = backtrace_symbols(bt, bt_size);              \
            if (bt_syms) {                                                \
                printf("<" component ">: Stack trace:\n");                \
                for (int i = 0; i < bt_size; ++i) {                       \
                    printf("  [%d] %s\n", i, bt_syms[i]);                 \
                }                                                         \
                free(bt_syms);                                            \
            } else {                                                      \
                printf("<BGSlice>: Failed to get stack trace symbols\n"); \
            }                                                             \
        }                                                                 \
    } while (0)

#define bg_assert(component, condition, fmt, ...)                          \
    do {                                                                   \
        if (bg_unlikely(!(condition) && assert_enabled)) {                 \
            fprintf(stderr, "ASSERT: <%s>: at func %s(): %s:%d " fmt "\n", \
                    component, __func__, __FILE__,                         \
                    __LINE__ __VA_OPT__(, ) __VA_ARGS__);                  \
            __bg_abort__;                                                  \
        }                                                                  \
    } while (0)

#endif // COMMON_H