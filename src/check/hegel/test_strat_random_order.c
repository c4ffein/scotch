#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"
#include "scotch_helpers.h"

/*
** Property: randomly generated ordering strategy strings
** produce valid permutations on any graph.
*/
static
void
testStratRandomOrder (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          vertnum;
  char *              seen;
  char                stratbuf[1024];

  scotchReset ();
  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  stratGenOrder (tc, stratbuf, sizeof (stratbuf));

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrder (&stratdat, stratbuf) == 0,
                "stratGraphOrder('%s') failed to parse", stratbuf);

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;
  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, NULL, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder with strategy '%s' failed", stratbuf);

  seen = calloc (vertnbr, sizeof (char));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          p;

    p = permtab[vertnum] - baseval;
    HEGEL_ASSERT (p >= 0 && p < vertnbr,
                  "permtab[%d] = %d out of range (strategy '%s')",
                  (int) vertnum, (int) permtab[vertnum], stratbuf);
    HEGEL_ASSERT (!seen[p],
                  "permtab duplicate %d (strategy '%s')",
                  (int) permtab[vertnum], stratbuf);
    seen[p] = 1;
  }

  free (seen);
  free (permtab);
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

  printf ("Running randomized ordering strategy test...\n");
  hegel_run_test (testStratRandomOrder);
  printf ("PASSED\n");

  return (0);
}
