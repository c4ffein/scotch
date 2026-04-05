#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress: star graphs — one hub vertex connected to all others.
** Extreme degree imbalance (hub has degree n-1, leaves have degree 1).
*/
static
void
testStressStar (
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
  SCOTCH_Num *        colotab;
  SCOTCH_Num          colonbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;
  SCOTCH_Num          diamval;

  baseval = hegel_draw_int (tc, 0, 1);
  vertnbr = hegel_draw_int (tc, 10, 200);
  nparts  = hegel_draw_int (tc, 2, 8);
  edgenbr = 2 * (vertnbr - 1);  /* hub->leaf + leaf->hub */

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));

  edgenum = 0;

  /* Hub vertex 0: connected to all others */
  verttab[0] = edgenum + baseval;
  for (vertnum = 1; vertnum < vertnbr; vertnum ++)
    edgetab[edgenum ++] = vertnum + baseval;

  /* Leaf vertices: connected only to hub */
  for (vertnum = 1; vertnum < vertnbr; vertnum ++) {
    verttab[vertnum] = edgenum + baseval;
    edgetab[edgenum ++] = 0 + baseval;
  }
  verttab[vertnbr] = edgenum + baseval;

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed (star_%d)", (int) vertnbr);

  /* Partition */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed on star_%d", (int) vertnbr);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d", (int) vertnum, (int) parttab[vertnum]);
  }
  free (parttab);
  SCOTCH_stratExit (&stratdat);

  /* Color — star is bipartite, so chromatic number is 2 */
  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed on star_%d", (int) vertnbr);
  /* Just check proper coloring — hub must differ from all leaves */
  for (vertnum = 1; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (colotab[0] != colotab[vertnum],
                  "hub and leaf %d share color %d",
                  (int) vertnum, (int) colotab[0]);
  }
  free (colotab);

  /* Diameter of a star is 2 (hub-to-any is 1, leaf-to-leaf is 2) */
  diamval = SCOTCH_graphDiamPV (&grafdat);
  HEGEL_ASSERT (diamval == 2,
                "star_%d diameter: expected 2, got %d",
                (int) vertnbr, (int) diamval);

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

  printf ("Running star graph stress test...\n");
  hegel_run_test (testStressStar);
  printf ("PASSED\n");

  return (0);
}
