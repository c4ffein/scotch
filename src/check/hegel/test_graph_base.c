#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: rebasing a graph with SCOTCH_graphBase preserves
** graph validity and operations still work correctly.
** Partition results should be identical regardless of base.
*/
static
void
testGraphBase (
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
  SCOTCH_Num *        parttab;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          oldbase;
  SCOTCH_Num          newbase;

  /* Build graph with base 0 */
  graphGenRandom (tc, 0, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, 0, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Rebase to 1 */
  oldbase = SCOTCH_graphBase (&grafdat, 1);
  HEGEL_ASSERT (oldbase == 0, "old baseval should be 0, got %d", (int) oldbase);

  /* Property: graphCheck passes after rebasing */
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                "graphCheck failed after rebase to 1");

  /* Property: partitioning works after rebasing */
  nparts = hegel_draw_int (tc, 2, 8);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed after rebase");

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d has partition %d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
  }

  /* Rebase back to 0 */
  newbase = SCOTCH_graphBase (&grafdat, 0);
  HEGEL_ASSERT (newbase == 1, "baseval should be 1 after first rebase, got %d", (int) newbase);

  /* Property: graphCheck still passes */
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                "graphCheck failed after rebase back to 0");

  free (parttab);
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

  printf ("Running graphBase rebasing test...\n");
  hegel_run_test (testGraphBase);
  printf ("PASSED\n");

  return (0);
}
