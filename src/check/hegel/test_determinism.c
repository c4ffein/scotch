#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: with deterministic mode and fixed seed,
** partitioning the same graph twice produces identical results.
*/
static
void
testDeterminism (
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
  SCOTCH_Num *        parttab1;
  SCOTCH_Num *        parttab2;
  SCOTCH_Num          vertnum;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 8);

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab1 = malloc (vertnbr * sizeof (SCOTCH_Num));
  parttab2 = malloc (vertnbr * sizeof (SCOTCH_Num));

  /* First run */
  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab1) == 0,
                "graphPart(1) failed");

  /* Second run with same seed */
  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab2) == 0,
                "graphPart(2) failed");

  /* Property: results must be identical */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab1[vertnum] == parttab2[vertnum],
                  "non-determinism at vertex %d: %d vs %d",
                  (int) vertnum, (int) parttab1[vertnum], (int) parttab2[vertnum]);
  }

  free (parttab2);
  free (parttab1);
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

  printf ("Running determinism property test...\n");
  hegel_run_test (testDeterminism);
  printf ("PASSED\n");

  return (0);
}
