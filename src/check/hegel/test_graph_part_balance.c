#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: partitioning with SCOTCH_STRATBALANCE flag via
** stratGraphMapBuild produces reasonably balanced partitions
** (no part has more than 2x the average number of vertices).
*/
static
void
testPartBalance (
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
  SCOTCH_Num *        partcnt;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          partnum;
  SCOTCH_Num          avgsize;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 4);
  /* Need enough vertices for meaningful balance check */
  hegel_assume (tc, vertnbr >= nparts * 4);

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat,
                                           SCOTCH_STRATBALANCE,
                                           nparts, 0.01) == 0,
                "stratGraphMapBuild failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");

  /* Count vertices per partition */
  partcnt = calloc (nparts, sizeof (SCOTCH_Num));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d has partition %d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
    partcnt[parttab[vertnum]] ++;
  }

  /* Property: no partition has more than 2x the average */
  avgsize = vertnbr / nparts;
  for (partnum = 0; partnum < nparts; partnum ++) {
    HEGEL_ASSERT (partcnt[partnum] <= 2 * avgsize + 2,
                  "partition %d has %d vertices, avg=%d (too unbalanced)",
                  (int) partnum, (int) partcnt[partnum], (int) avgsize);
  }

  /* Property: no partition is empty (each part should have at least 1 vertex) */
  for (partnum = 0; partnum < nparts; partnum ++) {
    HEGEL_ASSERT (partcnt[partnum] > 0,
                  "partition %d is empty", (int) partnum);
  }

  free (partcnt);
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

  printf ("Running balanced partition property test...\n");
  hegel_run_test (testPartBalance);
  printf ("PASSED\n");

  return (0);
}
