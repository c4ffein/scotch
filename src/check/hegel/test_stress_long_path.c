#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress: very long path graphs (500-2000 vertices).
** These are the worst case for nested dissection ordering
** (deep recursion) and for balanced partitioning (very narrow).
*/
static
void
testStressLongPath (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        parttab;
  SCOTCH_Num *        permtab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;
  SCOTCH_Num          diamval;

  baseval = hegel_draw_int (tc, 0, 1);
  vertnbr = hegel_draw_int (tc, 500, 2000);
  nparts  = hegel_draw_int (tc, 2, 16);
  edgenbr = 2 * (vertnbr - 1);

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    verttab[vertnum] = edgenum + baseval;
    if (vertnum > 0)
      edgetab[edgenum ++] = (vertnum - 1) + baseval;
    if (vertnum < vertnbr - 1)
      edgetab[edgenum ++] = (vertnum + 1) + baseval;
  }
  verttab[vertnbr] = edgenum + baseval;

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed (path_%d)", (int) vertnbr);

  /* Partition */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed on path_%d", (int) vertnbr);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d", (int) vertnum, (int) parttab[vertnum]);
  }
  free (parttab);
  SCOTCH_stratExit (&stratdat);

  /* Order */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit(2) failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                "stratGraphOrderBuild failed");

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;
  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, NULL, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder failed on path_%d", (int) vertnbr);

  {
    char *              seen;

    seen = calloc (vertnbr, sizeof (char));
    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      SCOTCH_Num          p;

      p = permtab[vertnum] - baseval;
      HEGEL_ASSERT (p >= 0 && p < vertnbr, "permtab out of range");
      HEGEL_ASSERT (!seen[p], "permtab duplicate");
      seen[p] = 1;
    }
    free (seen);
  }

  free (permtab);
  SCOTCH_stratExit (&stratdat);

  /* Diameter of path = vertnbr - 1 */
  diamval = SCOTCH_graphDiamPV (&grafdat);
  HEGEL_ASSERT (diamval == vertnbr - 1,
                "path_%d diameter: expected %d, got %d",
                (int) vertnbr, (int) (vertnbr - 1), (int) diamval);

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

  printf ("Running long path stress test (500-2000 vertices)...\n");
  hegel_run_test_n (testStressLongPath, 10);
  printf ("PASSED\n");

  return (0);
}
