#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Build a disconnected graph from two separate path components.
*/
static
void
graphGenDisconnected (
hegel_testcase *            tc,
const SCOTCH_Num            baseval,
SCOTCH_Num *                vertnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          sizeA, sizeB;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  sizeA = hegel_draw_int (tc, 2, 15);
  sizeB = hegel_draw_int (tc, 2, 15);
  vertnbr = sizeA + sizeB;
  edgenbr = 2 * (sizeA - 1) + 2 * (sizeB - 1);

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (vertnum = 0; vertnum < sizeA; vertnum ++) {
    verttab[vertnum] = edgenum + baseval;
    if (vertnum > 0)
      edgetab[edgenum ++] = (vertnum - 1) + baseval;
    if (vertnum < sizeA - 1)
      edgetab[edgenum ++] = (vertnum + 1) + baseval;
  }
  for (vertnum = sizeA; vertnum < vertnbr; vertnum ++) {
    verttab[vertnum] = edgenum + baseval;
    if (vertnum > sizeA)
      edgetab[edgenum ++] = (vertnum - 1) + baseval;
    if (vertnum < vertnbr - 1)
      edgetab[edgenum ++] = (vertnum + 1) + baseval;
  }
  verttab[vertnbr] = edgenum + baseval;

  *vertnbrptr = vertnbr;
  *verttabptr = verttab;
  *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Property: partition, coloring, and ordering all succeed on a
** disconnected graph.
*/
static
void
testDisconnected (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num *        parttab;
  SCOTCH_Num *        colotab;
  SCOTCH_Num          colonbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num *        peritab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num *        verttab_out;
  SCOTCH_Num *        edgetab_out;
  char *              seen;

  baseval = hegel_draw_int (tc, 0, 1);
  nparts  = hegel_draw_int (tc, 2, 8);

  graphGenDisconnected (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* --- Partition --- */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
  }
  free (parttab);
  SCOTCH_stratExit (&stratdat);

  /* --- Coloring --- */
  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed");

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
                    "adjacent %d and %d share color %d",
                    (int) vertnum, (int) vertend, (int) colotab[vertnum]);
    }
  }
  free (colotab);

  /* --- Ordering (use proper ordering strategy) --- */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit(2) failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat,
                  SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                "stratGraphOrderBuild failed");

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  peritab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;

  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, peritab, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder failed");

  seen = calloc (vertnbr, sizeof (char));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (permtab[vertnum] >= baseval && permtab[vertnum] < baseval + vertnbr,
                  "permtab[%d] = %d out of range",
                  (int) vertnum, (int) permtab[vertnum]);
    HEGEL_ASSERT (!seen[permtab[vertnum] - baseval],
                  "permtab duplicate %d", (int) permtab[vertnum]);
    seen[permtab[vertnum] - baseval] = 1;
  }
  free (seen);

  free (peritab);
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

  printf ("Running disconnected graph property test...\n");
  hegel_run_test (testDisconnected);
  printf ("PASSED\n");

  return (0);
}
