#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: a graph with exactly 1 vertex and 0 edges behaves correctly
** for graphCheck, graphPart, graphColor, and graphOrder.
*/
static
void
testSingleVertex (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          verttab[2];
  SCOTCH_Num          edgetab[1];
  SCOTCH_Num          parttab[1];
  SCOTCH_Num          colotab[1];
  SCOTCH_Num          colonbr;
  SCOTCH_Num          permtab[1];
  SCOTCH_Num          peritab[1];
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          nparts;

  baseval = hegel_draw_int (tc, 0, 1);
  nparts  = hegel_draw_int (tc, 1, 4);

  verttab[0] = baseval;
  verttab[1] = baseval;
  edgetab[0] = 0;

  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&grafdat, baseval, 1,
                                   verttab, NULL, NULL, NULL,
                                   0, edgetab, NULL) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0, "graphCheck failed");

  /* --- graphPart --- */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");
  HEGEL_ASSERT (parttab[0] >= 0 && parttab[0] < nparts,
                "partition = %d, expected [0, %d)",
                (int) parttab[0], (int) nparts);
  SCOTCH_stratExit (&stratdat);

  /* --- graphColor --- */
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed");
  HEGEL_ASSERT (colotab[0] == 0, "color = %d, expected 0", (int) colotab[0]);
  HEGEL_ASSERT (colonbr == 1, "colonbr = %d, expected 1", (int) colonbr);

  /* --- graphOrder (needs a proper ordering strategy) --- */
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit(2) failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, SCOTCH_STRATDEFAULT, 0, 0.2) == 0,
                "stratGraphOrderBuild failed");

  cblknbr = 0;
  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, peritab, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder failed");
  HEGEL_ASSERT (permtab[0] == baseval,
                "permtab[0] = %d, expected %d", (int) permtab[0], (int) baseval);

  SCOTCH_stratExit (&stratdat);
  SCOTCH_graphExit (&grafdat);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running single-vertex graph property test...\n");
  hegel_run_test (testSingleVertex);
  printf ("PASSED\n");

  return (0);
}
