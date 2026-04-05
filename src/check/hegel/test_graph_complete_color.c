#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: coloring a complete graph K_n must use exactly n colors,
** since the chromatic number of K_n is n.
*/
static
void
testCompleteColor (
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
  graphGenComplete (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed on K_%d", (int) vertnbr);

  /* Property: chromatic number of K_n is exactly n */
  HEGEL_ASSERT (colonbr == vertnbr,
                "K_%d colored with %d colors, expected exactly %d",
                (int) vertnbr, (int) colonbr, (int) vertnbr);

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
                    "K_%d: adjacent vertices %d and %d share color %d",
                    (int) vertnbr, (int) vertnum, (int) vertend,
                    (int) colotab[vertnum]);
    }
  }

  /* Property: all n colors are used (each vertex gets unique color) */
  {
    char *              seen;
    SCOTCH_Num          colornum;
    SCOTCH_Num          seencnt;

    seen = calloc (colonbr, sizeof (char));
    for (vertnum = 0; vertnum < vertnbr; vertnum ++)
      seen[colotab[vertnum]] = 1;

    seencnt = 0;
    for (colornum = 0; colornum < colonbr; colornum ++) {
      if (seen[colornum])
        seencnt ++;
    }
    HEGEL_ASSERT (seencnt == vertnbr,
                  "K_%d: only %d of %d colors actually used",
                  (int) vertnbr, (int) seencnt, (int) vertnbr);
    free (seen);
  }

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

  printf ("Running complete graph coloring property test...\n");
  hegel_run_test (testCompleteColor);
  printf ("PASSED\n");

  return (0);
}
