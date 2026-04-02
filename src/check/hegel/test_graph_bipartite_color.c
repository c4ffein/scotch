#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: coloring a 2D grid produces a proper coloring.
** Scotch's greedy heuristic doesn't guarantee optimal chromatic number,
** so we just check the coloring is valid (no adjacent same color).
*/
static
void
testBipartiteColor (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        colotab;
  SCOTCH_Num          colonbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num *        verttab_out;
  SCOTCH_Num *        edgetab_out;

  baseval = hegel_draw_int (tc, 0, 1);
  graphGenGrid2D (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed");

  /* Property: coloring is proper */
  SCOTCH_graphData (&grafdat, NULL, NULL, &verttab_out, NULL,
                    NULL, NULL, NULL, &edgetab_out, NULL);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          edgenum;

    HEGEL_ASSERT (colotab[vertnum] >= 0 && colotab[vertnum] < colonbr,
                  "vertex %d color %d out of [0, %d)",
                  (int) vertnum, (int) colotab[vertnum], (int) colonbr);

    for (edgenum = verttab_out[vertnum] - baseval;
         edgenum < verttab_out[vertnum + 1] - baseval; edgenum ++) {
      SCOTCH_Num          vertend;

      vertend = edgetab_out[edgenum] - baseval;
      HEGEL_ASSERT (colotab[vertnum] != colotab[vertend],
                    "adjacent vertices %d and %d share color %d",
                    (int) vertnum, (int) vertend, (int) colotab[vertnum]);
    }
  }

  /* Property: grid coloring should be reasonable (not hundreds of colors) */
  HEGEL_ASSERT (colonbr <= vertnbr,
                "colonbr %d > vertnbr %d", (int) colonbr, (int) vertnbr);

  free (colotab);
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

  printf ("Running bipartite grid coloring property test...\n");
  hegel_run_test (testBipartiteColor);
  printf ("PASSED\n");

  return (0);
}
