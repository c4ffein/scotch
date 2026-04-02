#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: graphInducePart produces a valid subgraph from a bipartition.
** First partition, then induce the subgraph of part 0.
*/
static
void
testInducePart (
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
  SCOTCH_GraphPart2 * part2tab;
  SCOTCH_Num          indvertnbr;
  SCOTCH_Num          indedgenbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          part0cnt;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Partition into 2 parts */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, 2, &stratdat, parttab) == 0,
                "graphPart failed");

  /* Convert to GraphPart2 and count part 0 */
  part2tab = malloc (vertnbr * sizeof (SCOTCH_GraphPart2));
  part0cnt = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    part2tab[vertnum] = (SCOTCH_GraphPart2) parttab[vertnum];
    if (parttab[vertnum] == 0)
      part0cnt ++;
  }

  hegel_assume (tc, part0cnt > 0); /* Need at least one vertex in part 0 */

  HEGEL_ASSERT (SCOTCH_graphInit (&indgrafdat) == 0, "graphInit(ind) failed");
  HEGEL_ASSERT (SCOTCH_graphInducePart (&grafdat, part0cnt, part2tab, 0, &indgrafdat) == 0,
                "graphInducePart failed");

  /* Property: induced graph passes graphCheck */
  HEGEL_ASSERT (SCOTCH_graphCheck (&indgrafdat) == 0,
                "graphCheck on induced graph failed");

  /* Property: induced graph has exactly part0cnt vertices */
  SCOTCH_graphSize (&indgrafdat, &indvertnbr, &indedgenbr);
  HEGEL_ASSERT (indvertnbr == part0cnt,
                "induced vertnbr: expected %d, got %d",
                (int) part0cnt, (int) indvertnbr);

  SCOTCH_graphExit (&indgrafdat);
  free (part2tab);
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

  printf ("Running graphInducePart property test...\n");
  hegel_run_test (testInducePart);
  printf ("PASSED\n");

  return (0);
}
