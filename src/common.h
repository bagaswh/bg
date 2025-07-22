#ifndef ASSERTION_H
#define ASSERTION_H

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

#endif // ASSERTION_H