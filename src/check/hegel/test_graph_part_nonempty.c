#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: when nparts <= vertnbr on a connected graph,
** all parts should be non-empty (used by at least one vertex).
** Use connected graph types only (grid, complete, path).
*/
static
void
testPartNonempty (
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
  int                 graphtype;

  baseval   = hegel_draw_int (tc, 0, 1);
  graphtype = hegel_draw_int (tc, 1, 3);  /* 1=grid, 2=complete, 3=path — all connected */

  switch (graphtype) {
    case 1:
      graphGenGrid2D (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      break;
    case 2:
      graphGenComplete (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      break;
    default:
      graphGenPath (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      break;
  }

  /* nparts must be <= vertnbr for non-empty guarantee */
  nparts = hegel_draw_int (tc, 2, vertnbr < 8 ? vertnbr : 8);

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat, SCOTCH_STRATBALANCE,
                                           nparts, 0.05) == 0,
                "stratGraphMapBuild failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");

  /* Count vertices per part */
  partcnt = calloc (nparts, sizeof (SCOTCH_Num));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d out of range",
                  (int) vertnum, (int) parttab[vertnum]);
    partcnt[parttab[vertnum]] ++;
  }

  /* Property: all parts non-empty */
  for (partnum = 0; partnum < nparts; partnum ++) {
    HEGEL_ASSERT (partcnt[partnum] > 0,
                  "part %d is empty (vertnbr=%d, nparts=%d)",
                  (int) partnum, (int) vertnbr, (int) nparts);
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

  printf ("Running partition non-empty parts test...\n");
  hegel_run_test (testPartNonempty);
  printf ("PASSED\n");

  return (0);
}
