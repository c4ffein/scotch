#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: two-step coarsening (CoarsenMatch + CoarsenBuild)
** produces a valid coarse graph.
*/
static
void
testCoarsenMatchBuild (
hegel_testcase *            tc)
{
  SCOTCH_Graph        finegrafdat;
  SCOTCH_Graph        coargrafdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        finematetab;
  SCOTCH_Num *        coarmulttab;
  SCOTCH_Num          coarvertnbr;
  SCOTCH_Num          coarvertnbr_out;
  int                 rc;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  hegel_assume (tc, edgenbr > 0);
  HEGEL_ASSERT (graphGenBuild (&finegrafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  finematetab = malloc (vertnbr * sizeof (SCOTCH_Num));
  coarvertnbr = 0;  /* 0 = no minimum target */

  /* Step 1: compute matching */
  rc = SCOTCH_graphCoarsenMatch (&finegrafdat, &coarvertnbr, 0.8,
                                 SCOTCH_COARSENNONE, finematetab);
  /* rc: 0 = success, 1 = threshold not met, 2 = error */
  HEGEL_ASSERT (rc != 2, "graphCoarsenMatch error");

  if (rc == 0) {
    /* Property: coarvertnbr > 0 and <= vertnbr */
    HEGEL_ASSERT (coarvertnbr > 0 && coarvertnbr <= vertnbr,
                  "coarvertnbr %d invalid (vertnbr=%d)",
                  (int) coarvertnbr, (int) vertnbr);

    /* Property: mate values are valid vertex indices */
    {
      SCOTCH_Num          vertnum;

      for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
        SCOTCH_Num          mate;

        mate = finematetab[vertnum];
        HEGEL_ASSERT (mate >= baseval && mate < baseval + vertnbr,
                      "mate[%d] = %d out of range [%d, %d)",
                      (int) vertnum, (int) mate,
                      (int) baseval, (int) (baseval + vertnbr));
      }
    }

    /* Step 2: build coarse graph */
    coarmulttab = malloc (coarvertnbr * 2 * sizeof (SCOTCH_Num));
    HEGEL_ASSERT (SCOTCH_graphInit (&coargrafdat) == 0, "graphInit(coar) failed");

    rc = SCOTCH_graphCoarsenBuild (&finegrafdat, coarvertnbr,
                                   finematetab, &coargrafdat, coarmulttab);
    HEGEL_ASSERT (rc == 0, "graphCoarsenBuild failed");

    /* Property: coarse graph passes graphCheck */
    HEGEL_ASSERT (SCOTCH_graphCheck (&coargrafdat) == 0,
                  "graphCheck on coarse graph failed");

    /* Property: coarse graph has correct vertex count */
    SCOTCH_graphSize (&coargrafdat, &coarvertnbr_out, NULL);
    HEGEL_ASSERT (coarvertnbr_out == coarvertnbr,
                  "coarse vertnbr: expected %d, got %d",
                  (int) coarvertnbr, (int) coarvertnbr_out);

    SCOTCH_graphExit (&coargrafdat);
    free (coarmulttab);
  }

  free (finematetab);
  SCOTCH_graphExit (&finegrafdat);
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

  printf ("Running two-step coarsening property test...\n");
  hegel_run_test (testCoarsenMatchBuild);
  printf ("PASSED\n");

  return (0);
}
