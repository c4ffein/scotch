#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress: dense graphs (high edge/vertex ratio).
** Complete graphs K_n for n up to 100.
*/
static
void
testStressDense (
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

  baseval = hegel_draw_int (tc, 0, 1);
  vertnbr = hegel_draw_int (tc, 20, 100);
  nparts  = hegel_draw_int (tc, 2, 16);
  edgenbr = vertnbr * (vertnbr - 1);

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));

  {
    SCOTCH_Num          edgenum;

    edgenum = 0;
    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      SCOTCH_Num          vertend;

      verttab[vertnum] = edgenum + baseval;
      for (vertend = 0; vertend < vertnbr; vertend ++) {
        if (vertend != vertnum)
          edgetab[edgenum ++] = vertend + baseval;
      }
    }
    verttab[vertnbr] = edgenum + baseval;
  }

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed (K_%d)", (int) vertnbr);

  /* Partition */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed on K_%d", (int) vertnbr);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d on K_%d",
                  (int) vertnum, (int) parttab[vertnum], (int) vertnbr);
  }
  free (parttab);
  SCOTCH_stratExit (&stratdat);

  /* Color — K_n requires exactly n colors */
  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed on K_%d", (int) vertnbr);
  HEGEL_ASSERT (colonbr == vertnbr,
                "K_%d colored with %d colors, expected %d",
                (int) vertnbr, (int) colonbr, (int) vertnbr);
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

  printf ("Running dense graph (K_n) stress test...\n");
  hegel_run_test_n (testStressDense, 10);
  printf ("PASSED\n");

  return (0);
}
