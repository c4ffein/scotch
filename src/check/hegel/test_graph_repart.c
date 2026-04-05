#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: graphRepart (repartitioning from an old partition)
** produces valid partition values in [0, nparts).
*/
static
void
testRepart (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num *        oldparttab;
  SCOTCH_Num *        newparttab;
  SCOTCH_Num          vertnum;
  double              emraval;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 4);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  /* First: compute an initial partition */
  oldparttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, oldparttab) == 0,
                "initial graphPart failed");

  /* Repartition with migration cost */
  newparttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  emraval = (double) hegel_draw_int (tc, 1, 100) / 100.0;

  HEGEL_ASSERT (SCOTCH_graphRepart (&grafdat, nparts, oldparttab,
                                    emraval, NULL, &stratdat, newparttab) == 0,
                "graphRepart failed");

  /* Property: all new partition values in [0, nparts) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (newparttab[vertnum] >= 0 && newparttab[vertnum] < nparts,
                  "vertex %d: part=%d, expected [0, %d)",
                  (int) vertnum, (int) newparttab[vertnum], (int) nparts);
  }

  free (newparttab);
  free (oldparttab);
  SCOTCH_stratExit (&stratdat);
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

  printf ("Running graphRepart property test...\n");
  hegel_run_test (testRepart);
  printf ("PASSED\n");

  return (0);
}
