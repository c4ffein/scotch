#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: strategies created by builder functions produce
** valid partitions and orderings when used.
*/
static
void
testStratUse (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  int                 optype;

  optype = hegel_draw_int (tc, 0, 1);
  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();

  if (optype == 0) {
    /* Mapping with various builder flags */
    SCOTCH_Num          nparts;
    SCOTCH_Num *        parttab;
    SCOTCH_Num          vertnum;
    int                 flagidx;
    int                 flags[4] = { SCOTCH_STRATDEFAULT, SCOTCH_STRATQUALITY,
                                     SCOTCH_STRATSPEED, SCOTCH_STRATBALANCE };

    flagidx = hegel_draw_int (tc, 0, 3);
    nparts  = hegel_draw_int (tc, 2, 4);

    HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat, flags[flagidx],
                                             nparts, 0.05) == 0,
                  "stratGraphMapBuild failed");

    parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
    HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                  "graphPart with builder strategy failed");

    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                    "vertex %d: part=%d, expected [0, %d)",
                    (int) vertnum, (int) parttab[vertnum], (int) nparts);
    }
    free (parttab);
  }
  else {
    /* Ordering with various builder flags */
    SCOTCH_Num *        permtab;
    SCOTCH_Num          cblknbr;
    SCOTCH_Num          vertnum;
    char *              seen;
    int                 flagidx;
    int                 flags[4] = { SCOTCH_STRATDEFAULT,
                                     SCOTCH_STRATDISCONNECTED,
                                     SCOTCH_STRATLEVELMAX,
                                     SCOTCH_STRATLEAFSIMPLE };
    const char *        flagnames[4] = { "DEFAULT", "DISCONNECTED",
                                         "LEVELMAX", "LEAFSIMPLE" };

    flagidx = hegel_draw_int (tc, 0, 3);

    HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, flags[flagidx],
                                               3, 0.2) == 0,
                  "stratGraphOrderBuild failed");

    permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
    cblknbr = 0;

    HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                     permtab, NULL, &cblknbr,
                                     NULL, NULL) == 0,
                  "graphOrder with builder strategy failed");

    seen = calloc (vertnbr, sizeof (char));
    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      SCOTCH_Num          p;

      p = permtab[vertnum] - baseval;
      HEGEL_ASSERT (p >= 0 && p < vertnbr,
                    "permtab[%d] = %d out of range (flag=%s, vertnbr=%d, baseval=%d)",
                    (int) vertnum, (int) permtab[vertnum],
                    flagnames[flagidx], (int) vertnbr, (int) baseval);
      HEGEL_ASSERT (!seen[p],
                    "permtab duplicate %d (flag=%s)",
                    (int) permtab[vertnum], flagnames[flagidx]);
      seen[p] = 1;
    }

    free (seen);
    free (permtab);
  }

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

  printf ("Running strategy builder usage test...\n");
  hegel_run_test (testStratUse);
  printf ("PASSED\n");

  return (0);
}
