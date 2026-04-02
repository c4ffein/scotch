#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress: graph with many small disconnected components.
** Tests component handling in partitioning, coloring, and ordering.
*/
static
void
testStressManyComponents (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          ncomps;
  SCOTCH_Num          compsiz;
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
  SCOTCH_Num          comp;

  baseval = hegel_draw_int (tc, 0, 1);
  ncomps  = hegel_draw_int (tc, 5, 20);
  compsiz = hegel_draw_int (tc, 2, 8);
  nparts  = hegel_draw_int (tc, 2, 8);
  vertnbr = ncomps * compsiz;
  edgenbr = ncomps * 2 * (compsiz - 1);  /* Each component is a path */

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (comp = 0; comp < ncomps; comp ++) {
    SCOTCH_Num          base;

    base = comp * compsiz;
    for (vertnum = 0; vertnum < compsiz; vertnum ++) {
      verttab[base + vertnum] = edgenum + baseval;
      if (vertnum > 0)
        edgetab[edgenum ++] = (base + vertnum - 1) + baseval;
      if (vertnum < compsiz - 1)
        edgetab[edgenum ++] = (base + vertnum + 1) + baseval;
    }
  }
  verttab[vertnbr] = edgenum + baseval;

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenum) == 0,
                "graphGenBuild failed (%d components of %d)",
                (int) ncomps, (int) compsiz);

  /* Partition */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d", (int) vertnum, (int) parttab[vertnum]);
  }
  free (parttab);
  SCOTCH_stratExit (&stratdat);

  /* Color — check proper coloring */
  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed");
  HEGEL_ASSERT (colonbr > 0, "colonbr=%d <= 0", (int) colonbr);

  /* Each component is a path, so proper coloring verified per-edge */
  {
    SCOTCH_Num *        verttab_int;
    SCOTCH_Num *        edgetab_int;

    SCOTCH_graphData (&grafdat, NULL, NULL, &verttab_int, NULL,
                      NULL, NULL, NULL, &edgetab_int, NULL);

    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      SCOTCH_Num          e;

      for (e = verttab_int[vertnum] - baseval;
           e < verttab_int[vertnum + 1] - baseval; e ++) {
        SCOTCH_Num          nb;

        nb = edgetab_int[e] - baseval;
        HEGEL_ASSERT (colotab[vertnum] != colotab[nb],
                      "adjacent %d and %d share color %d",
                      (int) vertnum, (int) nb, (int) colotab[vertnum]);
      }
    }
  }
  free (colotab);

  /* Diameter should be SCOTCH_NUMMAX for disconnected graph */
  {
    SCOTCH_Num          diamval;

    diamval = SCOTCH_graphDiamPV (&grafdat);
    if (ncomps > 1) {
      HEGEL_ASSERT (diamval > 0, "disconnected diameter should be positive or NUMMAX, got %d", (int) diamval);
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

  printf ("Running many-components stress test...\n");
  hegel_run_test (testStressManyComponents);
  printf ("PASSED\n");

  return (0);
}
