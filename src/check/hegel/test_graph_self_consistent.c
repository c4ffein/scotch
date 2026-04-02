#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Compute edge cut manually from a partition and the graph CSR.
*/
static
SCOTCH_Num
computeEdgeCut (
const SCOTCH_Num *          verttab,
const SCOTCH_Num *          edgetab,
const SCOTCH_Num *          parttab,
const SCOTCH_Num            vertnbr,
const SCOTCH_Num            baseval)
{
  SCOTCH_Num          cut;
  SCOTCH_Num          vertnum;

  cut = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          edgenum;

    for (edgenum = verttab[vertnum] - baseval;
         edgenum < verttab[vertnum + 1] - baseval; edgenum ++) {
      SCOTCH_Num          vertend;

      vertend = edgetab[edgenum] - baseval;
      if (parttab[vertnum] != parttab[vertend])
        cut ++;
    }
  }
  /* Each cut edge counted twice (once per endpoint) */
  return (cut / 2);
}

/*
** Property: partition a graph with default strategy, then with
** SCOTCH_STRATBALANCE. Both produce valid partitions, and the
** balanced edge cut is within a loose bound of the default.
*/
static
void
testSelfConsistent (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat1;
  SCOTCH_Strat        stratdat2;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num *        parttab1;
  SCOTCH_Num *        parttab2;
  SCOTCH_Num          cut1, cut2;
  SCOTCH_Num          vertnum;
  SCOTCH_Num *        verttab_out;
  SCOTCH_Num *        edgetab_out;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 4);

  /* Default strategy */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat1) == 0, "stratInit(1) failed");
  parttab1 = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat1, parttab1) == 0,
                "graphPart(1) failed");

  /* Balance strategy */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat2) == 0, "stratInit(2) failed");
  HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat2,
                SCOTCH_STRATBALANCE, nparts, 0.05) == 0,
                "stratGraphMapBuild failed");
  parttab2 = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat2, parttab2) == 0,
                "graphPart(2) failed");

  /* Validate both partitions */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab1[vertnum] >= 0 && parttab1[vertnum] < nparts,
                  "parttab1[%d] = %d out of range", (int) vertnum, (int) parttab1[vertnum]);
    HEGEL_ASSERT (parttab2[vertnum] >= 0 && parttab2[vertnum] < nparts,
                  "parttab2[%d] = %d out of range", (int) vertnum, (int) parttab2[vertnum]);
  }

  /* Compute edge cuts using the internal arrays */
  SCOTCH_graphData (&grafdat, NULL, NULL, &verttab_out, NULL,
                    NULL, NULL, NULL, &edgetab_out, NULL);

  cut1 = computeEdgeCut (verttab_out, edgetab_out, parttab1, vertnbr, baseval);
  cut2 = computeEdgeCut (verttab_out, edgetab_out, parttab2, vertnbr, baseval);

  /* Loose sanity bound: balanced cut should not be absurdly larger */
  HEGEL_ASSERT (cut2 <= cut1 * 3 + vertnbr,
                "balanced cut %d vastly exceeds default cut %d (vertnbr=%d)",
                (int) cut2, (int) cut1, (int) vertnbr);

  free (parttab2);
  free (parttab1);
  SCOTCH_stratExit (&stratdat2);
  SCOTCH_stratExit (&stratdat1);
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

  printf ("Running self-consistent partition property test...\n");
  hegel_run_test (testSelfConsistent);
  printf ("PASSED\n");

  return (0);
}
