#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <setjmp.h>

#ifndef bg_expect_assertion
// The third argument is how many assertion is expected to run.
#    define bg_expect_assertion(stuff_to_do, name)                             \
        do {                                                                   \
            volatile int assertions_run = 0;                                   \
            if (setjmp(test_resume_env) == 0) {                                \
                stuff_to_do                                                    \
            } else {                                                           \
                assertions_run++;                                              \
            }                                                                  \
            TEST_ASSERT_EQUAL_INT_MESSAGE(                                     \
                1, assertions_run, "Assertion did not run for " name);         \
            /* Reset the jmp_buf, otherwise bizarre thing could happen.        \
            That is, next place where longjmp(test_resume_env, 1) is called,   \
            it'll jump to the previously place where setjmp(test_resume_env)   \
            is called. Which could be in the previous test case or             \
            something.                                                         \
                                                                             \ \
            Now if you find some bizarre behaviour happening in your test      \
            cases, like an unexpected segfault in a place where it should      \
            not, try to check if the function performs an                      \
            unsatisfied assertion, which will do a longjmp to a cleared        \
            jmp_buf if not wrapped with bg_expect_assertion, which of course   \
            will cause segfault. */                                            \
            memset(&test_resume_env, 0, sizeof(test_resume_env));              \
        } while (0)
#endif

// Used to test if assertion works.
jmp_buf test_resume_env;

#endif // TESTUTILS_H