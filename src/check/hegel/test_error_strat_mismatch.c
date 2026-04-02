#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: using the wrong strategy type for an operation
** returns an error code, doesn't crash.
*/
static
void
testErrorStratMismatch (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  int                 testcase;
  int                 rc;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  testcase = hegel_draw_int (tc, 0, 1);

  switch (testcase) {
    case 0: {
      /* Build a mapping strategy, use it for ordering */
      SCOTCH_Num *        permtab;
      SCOTCH_Num          cblknbr;

      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat, SCOTCH_STRATDEFAULT, 4, 0.05) == 0,
                    "stratGraphMapBuild failed");

      permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
      cblknbr = 0;
      rc = SCOTCH_graphOrder (&grafdat, &stratdat,
                              permtab, NULL, &cblknbr, NULL, NULL);
      /* Property: returns error, doesn't crash */
      HEGEL_ASSERT (rc != 0,
                    "graphOrder with mapping strategy should fail but returned 0");

      free (permtab);
      SCOTCH_stratExit (&stratdat);
      break;
    }
    case 1: {
      /* Build an ordering strategy, use it for partitioning */
      SCOTCH_Num *        parttab;

      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                    "stratGraphOrderBuild failed");

      parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
      rc = SCOTCH_graphPart (&grafdat, 4, &stratdat, parttab);
      /* Property: returns error, doesn't crash */
      HEGEL_ASSERT (rc != 0,
                    "graphPart with ordering strategy should fail but returned 0");

      free (parttab);
      SCOTCH_stratExit (&stratdat);
      break;
    }
  }

  SCOTCH_graphExit (&grafdat);
  free (verttab);
  free (edgetab);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running strategy type mismatch error test...\n");
  hegel_run_test (testErrorStratMismatch);
  printf ("PASSED\n");

  return (0);
}
