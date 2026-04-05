#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Stress: extreme vertex/edge weights (near INT_MAX/2).
** Tests for integer overflow in weight summation.
*/
static
void
testStressHeavyWeights (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        velotab;
  SCOTCH_Num *        edlotab;
  SCOTCH_Num *        parttab;
  SCOTCH_Num          nparts;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          v;
  int                 weighttype;

  baseval = 0;
  graphGenGrid2D (tc, baseval, &vertnbr, &verttab, &edgetab, &edgenbr);

  weighttype = hegel_draw_int (tc, 0, 2);

  velotab = NULL;
  edlotab = NULL;

  switch (weighttype) {
    case 0: {                                     /* Very large vertex weights */
      velotab = malloc (vertnbr * sizeof (SCOTCH_Num));
      for (vertnum = 0; vertnum < vertnbr; vertnum ++)
        velotab[vertnum] = hegel_draw_int (tc, 100000, 1000000);
      break;
    }
    case 1: {                                     /* Very imbalanced vertex weights */
      velotab = malloc (vertnbr * sizeof (SCOTCH_Num));
      for (vertnum = 0; vertnum < vertnbr; vertnum ++)
        velotab[vertnum] = (vertnum == 0) ? 1000000 : 1;
      break;
    }
    case 2: {                                     /* Large symmetric edge weights */
      edlotab = calloc (edgenbr, sizeof (SCOTCH_Num));
      for (v = 0; v < vertnbr; v ++) {
        SCOTCH_Num          start, end, e;

        start = verttab[v] - baseval;
        end   = verttab[v + 1] - baseval;
        for (e = start; e < end; e ++) {
          if (edlotab[e] == 0) {
            SCOTCH_Num          neighbor;
            SCOTCH_Num          nstart, nend, ne;
            SCOTCH_Num          weight;

            weight   = hegel_draw_int (tc, 10000, 100000);
            edlotab[e] = weight;
            neighbor = edgetab[e] - baseval;
            nstart = verttab[neighbor] - baseval;
            nend   = verttab[neighbor + 1] - baseval;
            for (ne = nstart; ne < nend; ne ++) {
              if (edgetab[ne] - baseval == v) {
                edlotab[ne] = weight;
                break;
              }
            }
          }
        }
      }
      break;
    }
  }

  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&grafdat, baseval, vertnbr,
                                   verttab, NULL, velotab, NULL,
                                   edgenbr, edgetab, edlotab) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                "graphCheck failed with heavy weights");

  nparts = hegel_draw_int (tc, 2, 4);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed with heavy weights");

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d", (int) vertnum, (int) parttab[vertnum]);
  }

  free (parttab);
  SCOTCH_stratExit (&stratdat);
  SCOTCH_graphExit (&grafdat);
  if (velotab != NULL) free (velotab);
  if (edlotab != NULL) free (edlotab);
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

  printf ("Running heavy weights stress test...\n");
  hegel_run_test (testStressHeavyWeights);
  printf ("PASSED\n");

  return (0);
}
