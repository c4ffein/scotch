#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: ordering produces a valid permutation and passes orderCheck.
*/
static
void
testOrdering (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Ordering     ordedat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num *        peritab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          vertnum;
  char *              seen;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  peritab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;

  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, peritab, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder failed");

  /* Check permtab is a valid permutation of [baseval, baseval + vertnbr) */
  seen = calloc (vertnbr, sizeof (char));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (permtab[vertnum] >= baseval && permtab[vertnum] < baseval + vertnbr,
                  "permtab[%d] = %d out of range [%d, %d)",
                  (int) vertnum, (int) permtab[vertnum],
                  (int) baseval, (int) (baseval + vertnbr));
    HEGEL_ASSERT (!seen[permtab[vertnum] - baseval],
                  "permtab has duplicate value %d", (int) permtab[vertnum]);
    seen[permtab[vertnum] - baseval] = 1;
  }

  /* Check peritab is inverse of permtab */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (peritab[permtab[vertnum] - baseval] == vertnum + baseval,
                  "peritab[permtab[%d] - baseval] != %d",
                  (int) vertnum, (int) (vertnum + baseval));
  }

  /* Use Scotch's own orderCheck */
  HEGEL_ASSERT (SCOTCH_graphOrderInit (&grafdat, &ordedat,
                                       permtab, peritab, &cblknbr,
                                       NULL, NULL) == 0,
                "graphOrderInit failed");
  HEGEL_ASSERT (SCOTCH_graphOrderCompute (&grafdat, &ordedat, &stratdat) == 0,
                "graphOrderCompute failed");
  HEGEL_ASSERT (SCOTCH_graphOrderCheck (&grafdat, &ordedat) == 0,
                "graphOrderCheck failed");
  SCOTCH_graphOrderExit (&grafdat, &ordedat);

  free (seen);
  free (peritab);
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

  printf ("Running ordering property test...\n");
  hegel_run_test (testOrdering);
  printf ("PASSED\n");

  return (0);
}
