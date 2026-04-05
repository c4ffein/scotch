#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: partitioning into nparts > vertnbr still succeeds,
** values are in [0, nparts), and the number of non-empty parts <= vertnbr.
*/
static
void
testManyParts (
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
  char *              used;
  SCOTCH_Num          usedcnt;
  SCOTCH_Num          partnum;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Choose nparts strictly greater than vertnbr */
  nparts = vertnbr + hegel_draw_int (tc, 1, 20);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed with nparts=%d > vertnbr=%d",
                (int) nparts, (int) vertnbr);

  /* Property: all values in [0, nparts) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d has partition %d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
  }

  /* Property: number of non-empty parts <= vertnbr */
  used = calloc (nparts, sizeof (char));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++)
    used[parttab[vertnum]] = 1;

  usedcnt = 0;
  for (partnum = 0; partnum < nparts; partnum ++) {
    if (used[partnum])
      usedcnt ++;
  }
  HEGEL_ASSERT (usedcnt <= vertnbr,
                "non-empty parts = %d > vertnbr = %d",
                (int) usedcnt, (int) vertnbr);

  free (used);
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

  printf ("Running many-parts partition property test...\n");
  hegel_run_test (testManyParts);
  printf ("PASSED\n");

  return (0);
}
