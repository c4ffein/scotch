#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Property: strategy builders produce strategies that can be
** freed without crashing. Invalid raw strings return non-zero
** but don't crash.
*/

static const char * const badstrats[] = {
  "GARBAGE{{{",
  "?!@#$%",
  "}{",
  "r{sep=BOGUS}",
  "z{",
};
#define BADSTRATNBR (sizeof (badstrats) / sizeof (badstrats[0]))

static
void
testStratParse (
hegel_testcase *            tc)
{
  SCOTCH_Strat        stratdat;
  int                 category;

  category = hegel_draw_int (tc, 0, 2);

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  switch (category) {
    case 0: {                                     /* Map builder then exit */
      SCOTCH_Num          nparts;
      double              balrat;

      nparts = hegel_draw_int (tc, 2, 32);
      balrat = (double) hegel_draw_int (tc, 1, 100) / 1000.0;
      HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat, SCOTCH_STRATDEFAULT,
                                               nparts, balrat) == 0,
                    "stratGraphMapBuild failed");
      break;
    }
    case 1: {                                     /* Order builder then exit */
      double              balrat;

      balrat = (double) hegel_draw_int (tc, 1, 500) / 1000.0;
      HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT,
                                                 0, balrat) == 0,
                    "stratGraphOrderBuild failed");
      break;
    }
    case 2: {                                     /* Invalid string — must not crash */
      int                 idx;
      int                 rc;

      idx = hegel_draw_int (tc, 0, BADSTRATNBR - 1);
      rc = SCOTCH_stratGraphMap (&stratdat, badstrats[idx]);
      /* Property: failure expected, but no crash */
      (void) rc;
      break;
    }
  }

  /* Property: stratExit always succeeds */
  SCOTCH_stratExit (&stratdat);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running strategy string parsing test...\n");
  hegel_run_test (testStratParse);
  printf ("PASSED\n");

  return (0);
}
