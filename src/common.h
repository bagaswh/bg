#ifndef COMMON_H
#define COMMON_H

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

#define print_backtrace()                                         \
  do {                                                            \
    if (unlikely(print_stacktrace_before_abort)) {                \
      void *bt[32];                                               \
      int bt_size = backtrace(bt, 32);                            \
      char **bt_syms = backtrace_symbols(bt, bt_size);            \
      if (bt_syms) {                                              \
        printf("<BGSlice>: Stack trace:\n");                      \
        for (int i = 0; i < bt_size; ++i) {                       \
          printf("  [%d] %s\n", i, bt_syms[i]);                   \
        }                                                         \
        free(bt_syms);                                            \
      } else {                                                    \
        printf("<BGSlice>: Failed to get stack trace symbols\n"); \
      }                                                           \
    }                                                             \
  } while (0)

#define bg_assert(component, condition, fmt, ...)                              \
  do {                                                                         \
    if (unlikely(!(condition) && assert_enabled)) {                            \
      fprintf(stderr, "ASSERT: <%s>: %s:%s:%d " fmt "\n", component, __FILE__, \
              __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);                  \
      print_backtrace();                                                       \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif  // COMMON_H