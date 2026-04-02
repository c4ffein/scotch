#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: overlap partitioning produces valid part values.
** In overlap partitioning, vertices belong to parts [0, partnbr)
** or to the "overlap" part (partnbr), which means the vertex
** is shared between multiple parts.
*/
static
void
testPartOvl (
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
  SCOTCH_Num          vertnum;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 8);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPartOvl (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPartOvl failed");

  /* Property: partition values are either in [0, nparts) for exclusive
  ** assignment, or negative (-1) for overlap vertices. */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] == -1 ||
                  (parttab[vertnum] >= 0 && parttab[vertnum] < nparts),
                  "vertex %d has part %d, expected -1 or [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
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

  printf ("Running overlap partition property test...\n");
  hegel_run_test (testPartOvl);
  printf ("PASSED\n");

  return (0);
}
