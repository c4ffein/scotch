#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"
#include "scotch_helpers.h"

/*
** Stress: partition, color, and order 10K-50K vertex grids.
** 5 iterations per test due to high cost per case.
*/
static
void
testStress10k (
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
  int                 optype;

  scotchReset ();
  baseval = hegel_draw_int (tc, 0, 1);
  optype  = hegel_draw_int (tc, 0, 2);
  dim0    = hegel_draw_int (tc, 100, 200);
  dim1    = hegel_draw_int (tc, 100, 200);
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
                "graphGenBuild failed (%dx%d = %d vertices)",
                (int) dim0, (int) dim1, (int) vertnbr);

  switch (optype) {
    case 0: {
      nparts = hegel_draw_int (tc, 4, 64);
      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
      HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                    "graphPart failed (%d vertices, %d parts)",
                    (int) vertnbr, (int) nparts);

      for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
        HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                      "vertex %d: part=%d", (int) vertnum, (int) parttab[vertnum]);
      }
      free (parttab);
      SCOTCH_stratExit (&stratdat);
      break;
    }
    case 1: {
      colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
      colonbr = 0;
      HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                    "graphColor failed (%d vertices)", (int) vertnbr);
      HEGEL_ASSERT (colonbr > 0 && colonbr <= vertnbr,
                    "colonbr=%d", (int) colonbr);
      free (colotab);
      break;
    }
    case 2: {
      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                    "stratGraphOrderBuild failed");

      permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
      cblknbr = 0;
      HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                       permtab, NULL, &cblknbr,
                                       NULL, NULL) == 0,
                    "graphOrder failed (%d vertices)", (int) vertnbr);

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
      break;
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

  printf ("Running 10K-40K vertex stress test...\n");
  hegel_run_test_n (testStress10k, 5);
  printf ("PASSED\n");

  return (0);
}
