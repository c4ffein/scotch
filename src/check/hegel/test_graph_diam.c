#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: graphDiamPV returns positive value for connected graphs,
** and SCOTCH_NUMMAX for disconnected graphs.
** Grid, path, and complete graphs are always connected.
*/
static
void
testDiamConnected (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          diamval;
  int                 graphtype;

  /* Generate a known-connected graph type */
  baseval   = hegel_draw_int (tc, 0, 1);
  graphtype = hegel_draw_int (tc, 1, 3); /* 1=grid, 2=complete, 3=path — all connected */

  switch (graphtype) {
    case GRAPHGEN_GRID2D:
      graphGenGrid2D (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      break;
    case GRAPHGEN_COMPLETE:
      graphGenComplete (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      break;
    default:
      graphGenPath (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      break;
  }

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  diamval = SCOTCH_graphDiamPV (&grafdat);

  /* Property: diameter is positive for connected graphs */
  HEGEL_ASSERT (diamval > 0, "diamPV returned %d for connected graph", (int) diamval);

  /* Property: not an error */
  HEGEL_ASSERT (diamval != -1, "diamPV returned -1 (error)");

  /* Property: diameter of a path of n vertices is n-1 */
  if (graphtype == GRAPHGEN_PATH) {
    HEGEL_ASSERT (diamval == vertnbr - 1,
                  "path diameter: expected %d, got %d",
                  (int) (vertnbr - 1), (int) diamval);
  }

  /* Property: diameter of a complete graph is 1 */
  if (graphtype == GRAPHGEN_COMPLETE && vertnbr >= 2) {
    HEGEL_ASSERT (diamval == 1,
                  "complete graph diameter: expected 1, got %d", (int) diamval);
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

  printf ("Running graphDiamPV property test...\n");
  hegel_run_test (testDiamConnected);
  printf ("PASSED\n");

  return (0);
}
