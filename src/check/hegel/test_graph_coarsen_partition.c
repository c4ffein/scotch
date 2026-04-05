#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: coarsen a graph, then partition the coarse graph.
** The coarse partition is valid (values in [0, nparts)), and
** when projected back to the fine graph via the multinode table,
** all fine vertex values are also in range.
*/
static
void
testCoarsenPartition (
hegel_testcase *            tc)
{
  SCOTCH_Graph        finegrafdat;
  SCOTCH_Graph        coargrafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        coarmulttab;
  SCOTCH_Num          coarvertnbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num *        coarparttab;
  SCOTCH_Num *        fineparttab;
  SCOTCH_Num          vertnum;
  int                 rc;

  /* Use base 0 for simpler multinode indexing */
  graphGenRandom (tc, 0, &vertnbr, &verttab, &edgetab, &edgenbr);
  baseval = 0;

  /* Need enough edges for coarsening to succeed */
  hegel_assume (tc, edgenbr >= vertnbr);

  HEGEL_ASSERT (SCOTCH_graphInit (&finegrafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&finegrafdat, baseval, vertnbr,
                                   verttab, NULL, NULL, NULL,
                                   edgenbr, edgetab, NULL) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&finegrafdat) == 0, "graphCheck failed");

  /* Draw nparts BEFORE coarsen to keep draw sequence deterministic */
  nparts = hegel_draw_int (tc, 2, 4);

  coarmulttab = malloc (vertnbr * 2 * sizeof (SCOTCH_Num));

  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
  rc = SCOTCH_graphCoarsen (&finegrafdat, 1, 0.8, SCOTCH_COARSENNONE,
                            &coargrafdat, coarmulttab);

  HEGEL_ASSERT (rc != 2, "graphCoarsen returned error (2)");
  hegel_assume (tc, rc == 0);                       /* Skip if threshold not met */

  HEGEL_ASSERT (SCOTCH_graphCheck (&coargrafdat) == 0,
                "graphCheck on coarse graph failed");

  SCOTCH_graphSize (&coargrafdat, &coarvertnbr, NULL);
  HEGEL_ASSERT (coarvertnbr <= vertnbr,
                "coarse vertnbr %d > fine vertnbr %d",
                (int) coarvertnbr, (int) vertnbr);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  coarparttab = malloc (coarvertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&coargrafdat, nparts, &stratdat, coarparttab) == 0,
                "graphPart on coarse graph failed");

  /* Property: coarse partition values in [0, nparts) */
  for (vertnum = 0; vertnum < coarvertnbr; vertnum ++) {
    HEGEL_ASSERT (coarparttab[vertnum] >= 0 && coarparttab[vertnum] < nparts,
                  "coarse vertex %d: part=%d, expected [0, %d)",
                  (int) vertnum, (int) coarparttab[vertnum], (int) nparts);
  }

  /* Project partition back to fine graph via multinode table.
  ** coarmulttab[2*c + 0] and coarmulttab[2*c + 1] are the fine
  ** vertices merged into coarse vertex c (second may equal first). */
  fineparttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  memset (fineparttab, -1, vertnbr * sizeof (SCOTCH_Num));

  for (vertnum = 0; vertnum < coarvertnbr; vertnum ++) {
    SCOTCH_Num          fine0, fine1;

    fine0 = coarmulttab[2 * vertnum];
    fine1 = coarmulttab[2 * vertnum + 1];

    HEGEL_ASSERT (fine0 >= 0 && fine0 < vertnbr,
                  "multinode[%d].0 = %d out of range",
                  (int) vertnum, (int) fine0);
    HEGEL_ASSERT (fine1 >= 0 && fine1 < vertnbr,
                  "multinode[%d].1 = %d out of range",
                  (int) vertnum, (int) fine1);

    fineparttab[fine0] = coarparttab[vertnum];
    fineparttab[fine1] = coarparttab[vertnum];
  }

  /* Property: all fine vertices got assigned */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (fineparttab[vertnum] >= 0 && fineparttab[vertnum] < nparts,
                  "fine vertex %d: projected part=%d, expected [0, %d)",
                  (int) vertnum, (int) fineparttab[vertnum], (int) nparts);
  }

  free (fineparttab);
  free (coarparttab);
  SCOTCH_stratExit (&stratdat);
  SCOTCH_graphExit (&coargrafdat);
  free (coarmulttab);
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

  printf ("Running coarsen+partition property test...\n");
  hegel_run_test (testCoarsenPartition);
  printf ("PASSED\n");

  return (0);
}
