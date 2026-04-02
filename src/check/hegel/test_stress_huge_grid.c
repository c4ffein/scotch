#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress: partition, color, and order grids with 1000-2500 vertices.
** Runs fewer hegel iterations (10) since each case is expensive.
*/
static
void
testHugeGrid (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          dim0, dim1, vertnbr, edgenbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num *        parttab;
  SCOTCH_Num *        colotab;
  SCOTCH_Num          colonbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  baseval = hegel_draw_int (tc, 0, 1);
  dim0 = hegel_draw_int (tc, 30, 50);
  dim1 = hegel_draw_int (tc, 30, 50);
  vertnbr = dim0 * dim1;
  edgenbr = 2 * ((dim0 - 1) * dim1 + dim0 * (dim1 - 1));

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          x, y;

    x = vertnum % dim0;
    y = vertnum / dim0;
    verttab[vertnum] = edgenum + baseval;
    if (x > 0)        edgetab[edgenum ++] = (y * dim0 + (x - 1)) + baseval;
    if (x < dim0 - 1) edgetab[edgenum ++] = (y * dim0 + (x + 1)) + baseval;
    if (y > 0)        edgetab[edgenum ++] = ((y - 1) * dim0 + x) + baseval;
    if (y < dim1 - 1) edgetab[edgenum ++] = ((y + 1) * dim0 + x) + baseval;
  }
  verttab[vertnbr] = edgenum + baseval;

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenum) == 0,
                "graphGenBuild failed (vertnbr=%d)", (int) vertnbr);

  /* Partition into many parts */
  nparts = hegel_draw_int (tc, 2, 32);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed (vertnbr=%d, nparts=%d)",
                (int) vertnbr, (int) nparts);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d", (int) vertnum, (int) parttab[vertnum]);
  }
  free (parttab);
  SCOTCH_stratExit (&stratdat);

  /* Color */
  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed (vertnbr=%d)", (int) vertnbr);

  HEGEL_ASSERT (colonbr > 0 && colonbr <= vertnbr,
                "colonbr=%d out of range", (int) colonbr);
  free (colotab);

  /* Order */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit(2) failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                "stratGraphOrderBuild failed");

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;
  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, NULL, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder failed (vertnbr=%d)", (int) vertnbr);

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

  printf ("Running huge grid stress test (1000-2500 vertices)...\n");
  hegel_run_test_n (testHugeGrid, 10);
  printf ("PASSED\n");

  return (0);
}
