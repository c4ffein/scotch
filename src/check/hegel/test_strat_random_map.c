#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"
#include "scotch_helpers.h"

/*
** Property: randomly generated mapping strategy strings
** produce valid partitions on any graph.
*/
static
void
testStratRandomMap (
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
  char                stratbuf[1024];

  scotchReset ();
  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 8);
  stratGenMap (tc, stratbuf, sizeof (stratbuf));

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_stratGraphMap (&stratdat, stratbuf) == 0,
                "stratGraphMap('%s') failed to parse", stratbuf);

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart with strategy '%s' failed", stratbuf);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d (strategy '%s')",
                  (int) vertnum, (int) parttab[vertnum], stratbuf);
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

  printf ("Running randomized mapping strategy test...\n");
  hegel_run_test (testStratRandomMap);
  printf ("PASSED\n");

  return (0);
}
