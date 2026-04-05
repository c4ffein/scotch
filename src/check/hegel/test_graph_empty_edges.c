#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: a graph with n vertices and 0 edges (independent set).
** Partition, coloring, and ordering all produce valid results.
*/
static
void
testEmptyEdges (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num          edgetab[1];
  SCOTCH_Num          nparts;
  SCOTCH_Num *        parttab;
  SCOTCH_Num *        colotab;
  SCOTCH_Num          colonbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num *        peritab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          vertnum;
  char *              seen;

  baseval = hegel_draw_int (tc, 0, 1);
  vertnbr = hegel_draw_int (tc, 2, 30);
  nparts  = hegel_draw_int (tc, 2, 8);

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  for (vertnum = 0; vertnum <= vertnbr; vertnum ++)
    verttab[vertnum] = baseval;
  edgetab[0] = 0;

  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&grafdat, baseval, vertnbr,
                                   verttab, NULL, NULL, NULL,
                                   0, edgetab, NULL) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0, "graphCheck failed");

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
  HEGEL_ASSERT (colonbr == 1,
                "edgeless: colonbr=%d, expected 1", (int) colonbr);
  free (colotab);

  /* --- Ordering (with proper ordering strategy) --- */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit(2) failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat,
                  SCOTCH_STRATDEFAULT | SCOTCH_STRATDISCONNECTED, 0, 0.2) == 0,
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
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running empty-edges graph property test...\n");
  hegel_run_test (testEmptyEdges);
  printf ("PASSED\n");

  return (0);
}
