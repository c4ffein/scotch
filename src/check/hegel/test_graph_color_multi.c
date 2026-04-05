#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: coloring the same graph multiple times with a fixed seed
** produces deterministic results (same number of colors), and every
** coloring is valid (no adjacent vertices share a color).
*/
static
void
testColorMulti (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        colotab1;
  SCOTCH_Num *        colotab2;
  SCOTCH_Num          colonbr1;
  SCOTCH_Num          colonbr2;
  SCOTCH_Num          vertnum;
  SCOTCH_Num *        verttab_out;
  SCOTCH_Num *        edgetab_out;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  colotab1 = malloc (vertnbr * sizeof (SCOTCH_Num));
  colotab2 = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr1 = 0;
  colonbr2 = 0;

  /* First coloring */
  SCOTCH_randomReset ();
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab1, &colonbr1, 0) == 0,
                "graphColor (1) failed");

  /* Second coloring with same seed */
  SCOTCH_randomReset ();
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab2, &colonbr2, 0) == 0,
                "graphColor (2) failed");

  /* Property: same number of colors */
  HEGEL_ASSERT (colonbr1 == colonbr2,
                "color count differs: %d vs %d",
                (int) colonbr1, (int) colonbr2);

  /* Property: colorings match (determinism with fixed seed) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (colotab1[vertnum] == colotab2[vertnum],
                  "vertex %d: color1=%d, color2=%d",
                  (int) vertnum, (int) colotab1[vertnum], (int) colotab2[vertnum]);
  }

  /* Property: both colorings are valid (no adjacent vertices share a color) */
  SCOTCH_graphData (&grafdat, NULL, NULL, &verttab_out, NULL, NULL, NULL, NULL, &edgetab_out, NULL);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          edgenum;

    HEGEL_ASSERT (colotab1[vertnum] >= 0 && colotab1[vertnum] < colonbr1,
                  "vertex %d has color %d, expected [0, %d)",
                  (int) vertnum, (int) colotab1[vertnum], (int) colonbr1);

    for (edgenum = verttab_out[vertnum] - baseval;
         edgenum < verttab_out[vertnum + 1] - baseval; edgenum ++) {
      SCOTCH_Num          vertend;

      vertend = edgetab_out[edgenum] - baseval;
      HEGEL_ASSERT (colotab1[vertnum] != colotab1[vertend],
                    "adjacent vertices %d and %d share color %d",
                    (int) vertnum, (int) vertend, (int) colotab1[vertnum]);
    }
  }

  free (colotab2);
  free (colotab1);
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

  printf ("Running multi-coloring property test...\n");
  hegel_run_test (testColorMulti);
  printf ("PASSED\n");

  return (0);
}
