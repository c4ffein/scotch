#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: partition → induce → partition chain.
** Partition a graph, induce subgraph of one part,
** re-partition the subgraph. All operations succeed
** and produce valid results.
*/
static
void
testComposePartInduce (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Graph        indgrafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        parttab;
  SCOTCH_Num *        indlisttab;
  SCOTCH_Num          indvertnbr;
  SCOTCH_Num *        indparttab;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          nparts2;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  /* Step 1: partition into 2 */
  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, 2, &stratdat, parttab) == 0,
                "graphPart failed");

  /* Collect vertices in part 0 */
  indlisttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  indvertnbr = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    if (parttab[vertnum] == 0)
      indlisttab[indvertnbr ++] = vertnum + baseval;
  }

  hegel_assume (tc, indvertnbr >= 2); /* Need at least 2 vertices to re-partition */

  /* Step 2: induce subgraph */
  HEGEL_ASSERT (SCOTCH_graphInit (&indgrafdat) == 0, "graphInit(ind) failed");
  HEGEL_ASSERT (SCOTCH_graphInduceList (&grafdat, indvertnbr, indlisttab, &indgrafdat) == 0,
                "graphInduceList failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&indgrafdat) == 0, "graphCheck(ind) failed");

  /* Step 3: re-partition the subgraph */
  nparts2 = hegel_draw_int (tc, 2, 4);
  indparttab = malloc (indvertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&indgrafdat, nparts2, &stratdat, indparttab) == 0,
                "graphPart(ind) failed");

  for (vertnum = 0; vertnum < indvertnbr; vertnum ++) {
    HEGEL_ASSERT (indparttab[vertnum] >= 0 && indparttab[vertnum] < nparts2,
                  "induced vertex %d: part=%d, expected [0, %d)",
                  (int) vertnum, (int) indparttab[vertnum], (int) nparts2);
  }

  free (indparttab);
  SCOTCH_graphExit (&indgrafdat);
  free (indlisttab);
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

  printf ("Running partition->induce->partition chain test...\n");
  hegel_run_test (testComposePartInduce);
  printf ("PASSED\n");

  return (0);
}
