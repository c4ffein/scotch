#ifndef HEGEL_C_H
#define HEGEL_C_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle — do not dereference in C */
typedef struct HegelTestCase hegel_testcase;

/* Run a property test: test_fn is called once per generated test case */
void hegel_run_test (void (*test_fn)(hegel_testcase *));
void hegel_run_test_n (void (*test_fn)(hegel_testcase *), uint64_t n_cases);

/* Draw random values within [min_val, max_val] */
int      hegel_draw_int   (hegel_testcase * tc, int min_val, int max_val);
int64_t  hegel_draw_i64   (hegel_testcase * tc, int64_t min_val, int64_t max_val);
uint64_t hegel_draw_u64   (hegel_testcase * tc, uint64_t min_val, uint64_t max_val);
size_t   hegel_draw_usize (hegel_testcase * tc, size_t min_val, size_t max_val);

/* If condition is 0, discard this test case (not a failure) */
void hegel_assume (hegel_testcase * tc, int condition);

/* Fail the test with a message — triggers hegel shrinking */
void hegel_fail (const char * msg);

/* Assert a condition — if false, fails with message and triggers shrinking */
void hegel_assert (int condition, const char * msg);

/*
** Convenience macro: HEGEL_ASSERT(cond, fmt, ...) formats a message and
** calls hegel_assert. Use this instead of assert() in hegel test functions.
*/
#define HEGEL_ASSERT(cond, ...) \
  do { \
    if (!(cond)) { \
      char _hegel_buf[512]; \
      snprintf (_hegel_buf, sizeof (_hegel_buf), __VA_ARGS__); \
      hegel_fail (_hegel_buf); \
    } \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* HEGEL_C_H */
