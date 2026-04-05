#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress test: partition, color, and order larger graphs (100-500 vertices).
** Same properties as the basic tests, just bigger inputs.
*/
static
void
testLargeGraph (
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
  SCOTCH_Num *        permtab;
  SCOTCH_Num *        peritab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num          vertnum;
  int                 optype;
  char *              seen;

  baseval = hegel_draw_int (tc, 0, 1);
  optype  = hegel_draw_int (tc, 0, 2);

  /* Generate a large grid */
  {
    SCOTCH_Num          dim0, dim1;

    dim0 = hegel_draw_int (tc, 10, 25);
    dim1 = hegel_draw_int (tc, 10, 25);
    vertnbr = dim0 * dim1;
    edgenbr = 2 * ((dim0 - 1) * dim1 + dim0 * (dim1 - 1));

    verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
    edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));

    {
      SCOTCH_Num          edgenum;

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
      edgenbr = edgenum;
    }
  }

  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  switch (optype) {
    case 0: {                                     /* Large partition */
      nparts = hegel_draw_int (tc, 2, 16);
      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

      parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
      HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                    "graphPart failed on large graph");

      for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
        HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                      "large graph: vertex %d part=%d, expected [0, %d)",
                      (int) vertnum, (int) parttab[vertnum], (int) nparts);
      }
      free (parttab);
      SCOTCH_stratExit (&stratdat);
      break;
    }
    case 1: {                                     /* Large coloring */
      colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
      colonbr = 0;
      HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                    "graphColor failed on large graph");

      for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
        HEGEL_ASSERT (colotab[vertnum] >= 0 && colotab[vertnum] < colonbr,
                      "large graph: vertex %d color=%d, expected [0, %d)",
                      (int) vertnum, (int) colotab[vertnum], (int) colonbr);
      }
      free (colotab);
      break;
    }
    case 2: {                                     /* Large ordering */
      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                    "stratGraphOrderBuild failed");

      permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
      peritab = malloc (vertnbr * sizeof (SCOTCH_Num));
      cblknbr = 0;
      HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                       permtab, peritab, &cblknbr,
                                       NULL, NULL) == 0,
                    "graphOrder failed on large graph");

      seen = calloc (vertnbr, sizeof (char));
      for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
        SCOTCH_Num          p;

        p = permtab[vertnum] - baseval;
        HEGEL_ASSERT (p >= 0 && p < vertnbr,
                      "large graph: permtab[%d] out of range", (int) vertnum);
        HEGEL_ASSERT (!seen[p], "large graph: permtab duplicate");
        seen[p] = 1;
      }
      free (seen);
      free (peritab);
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

  printf ("Running large graph stress test...\n");
  hegel_run_test (testLargeGraph);
  printf ("PASSED\n");

  return (0);
}
