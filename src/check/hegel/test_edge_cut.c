#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: edge cut computed manually from partition + adjacency
** must be non-negative and <= total edges.
** Also: if nparts == 1, edge cut must be 0.
*/
static
void
testEdgeCut (
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
  SCOTCH_Num *        verttab_int;
  SCOTCH_Num *        edgetab_int;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          cutcount;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 1, 6);   /* Include nparts=1 as edge case */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");

  /* Get internal arrays for neighbor traversal */
  SCOTCH_graphData (&grafdat, NULL, NULL, &verttab_int, NULL, NULL, NULL, NULL, &edgetab_int, NULL);

  /* Count cut edges (each cut edge counted once by checking u < v) */
  cutcount = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          edgenum;

    for (edgenum = verttab_int[vertnum] - baseval;
         edgenum < verttab_int[vertnum + 1] - baseval; edgenum ++) {
      SCOTCH_Num          vertend;

      vertend = edgetab_int[edgenum] - baseval;
      if (vertend > vertnum && parttab[vertnum] != parttab[vertend])
        cutcount ++;
    }
  }

  /* Property: cut is non-negative */
  HEGEL_ASSERT (cutcount >= 0, "negative edge cut: %d", (int) cutcount);

  /* Property: cut <= number of undirected edges */
  HEGEL_ASSERT (cutcount <= edgenbr / 2,
                "edge cut %d > total edges %d",
                (int) cutcount, (int) (edgenbr / 2));

  /* Property: if nparts == 1, cut must be 0 */
  if (nparts == 1) {
    HEGEL_ASSERT (cutcount == 0,
                  "nparts=1 but edge cut is %d", (int) cutcount);
  }

  free (parttab);
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

  printf ("Running edge cut property test...\n");
  hegel_run_test (testEdgeCut);
  printf ("PASSED\n");

  return (0);
}
