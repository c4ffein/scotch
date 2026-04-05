#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: partitioning weighted graphs produces valid results,
** and graphCheck passes on weighted graphs.
*/
static
void
testPartWeighted (
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
  SCOTCH_Num          nparts;
  SCOTCH_Num *        parttab;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;
  int                 use_velo;
  int                 use_edlo;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);

  /* Randomly decide whether to use vertex/edge weights */
  use_velo = hegel_draw_int (tc, 0, 1);
  use_edlo = hegel_draw_int (tc, 0, 1);

  velotab = NULL;
  edlotab = NULL;

  if (use_velo) {
    velotab = malloc (vertnbr * sizeof (SCOTCH_Num));
    for (vertnum = 0; vertnum < vertnbr; vertnum ++)
      velotab[vertnum] = hegel_draw_int (tc, 1, 100);
  }

  if (use_edlo && edgenbr > 0) {
    SCOTCH_Num          v;

    /* Edge weights must be symmetric: if edge (u,v) has weight W,
    ** then edge (v,u) must also have weight W. Walk the CSR and
    ** assign matching weights to each arc pair. */
    edlotab = calloc (edgenbr, sizeof (SCOTCH_Num));
    for (v = 0; v < vertnbr; v ++) {
      SCOTCH_Num          start, end, e;

      start = verttab[v] - baseval;
      end   = verttab[v + 1] - baseval;
      for (e = start; e < end; e ++) {
        if (edlotab[e] == 0) {                    /* Not yet assigned */
          SCOTCH_Num          neighbor;
          SCOTCH_Num          nstart, nend, ne;
          SCOTCH_Num          weight;

          weight   = hegel_draw_int (tc, 1, 100);
          edlotab[e] = weight;
          neighbor = edgetab[e] - baseval;

          /* Find reverse arc (neighbor -> v) and set same weight */
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
  }

  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&grafdat, baseval, vertnbr,
                                   verttab, NULL, velotab, NULL,
                                   edgenbr, edgetab, edlotab) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                "graphCheck failed on weighted graph");

  nparts = hegel_draw_int (tc, 2, 8);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed on weighted graph");

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d has partition %d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
  }

  free (parttab);
  SCOTCH_stratExit (&stratdat);
  SCOTCH_graphExit (&grafdat);
  if (velotab != NULL)
    free (velotab);
  if (edlotab != NULL)
    free (edlotab);
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

  printf ("Running weighted graph partition test...\n");
  hegel_run_test (testPartWeighted);
  printf ("PASSED\n");

  return (0);
}
