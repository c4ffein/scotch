#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: various edge-case and invalid inputs don't crash Scotch.
** We test return codes where possible, but the main property is: no segfault.
*/
static
void
testErrorGraphBuild (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          verttab_empty[1];
  SCOTCH_Num          edgetab_dummy[1];
  SCOTCH_Num          parttab[1];
  int                 testcase;
  int                 rc;

  testcase = hegel_draw_int (tc, 0, 4);

  switch (testcase) {
    case 0: {
      /* Empty graph: vertnbr=0, edgenbr=0 — should succeed */
      verttab_empty[0] = 0;
      edgetab_dummy[0] = 0;

      HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
      rc = SCOTCH_graphBuild (&grafdat, 0, 0,
                              verttab_empty, NULL, NULL, NULL,
                              0, edgetab_dummy, NULL);
      /* Property: doesn't crash. rc may be 0 or non-zero. */
      if (rc == 0) {
        /* If build succeeded, graphCheck should also pass */
        HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                      "graphCheck failed on empty graph");
      }
      SCOTCH_graphExit (&grafdat);
      break;
    }
    case 1: {
      /* nparts=1 partition: trivially all vertices go to part 0 */
      SCOTCH_Num          baseval;
      SCOTCH_Num          vertnbr;
      SCOTCH_Num *        verttab;
      SCOTCH_Num *        edgetab;
      SCOTCH_Num          edgenbr;
      SCOTCH_Num *        ptab;
      SCOTCH_Num          vertnum;

      graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                    "graphGenBuild failed");

      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      ptab = malloc (vertnbr * sizeof (SCOTCH_Num));
      rc = SCOTCH_graphPart (&grafdat, 1, &stratdat, ptab);
      HEGEL_ASSERT (rc == 0, "graphPart with nparts=1 failed");

      for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
        HEGEL_ASSERT (ptab[vertnum] == 0,
                      "nparts=1: vertex %d has part %d, expected 0",
                      (int) vertnum, (int) ptab[vertnum]);
      }

      free (ptab);
      SCOTCH_stratExit (&stratdat);
      SCOTCH_graphExit (&grafdat);
      free (verttab);
      free (edgetab);
      break;
    }
    case 2: {
      /* nparts=0 partition: should return error */
      SCOTCH_Num          baseval;
      SCOTCH_Num          vertnbr;
      SCOTCH_Num *        verttab;
      SCOTCH_Num *        edgetab;
      SCOTCH_Num          edgenbr;
      SCOTCH_Num *        ptab;

      graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
      HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                    "graphGenBuild failed");

      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      ptab = malloc (vertnbr * sizeof (SCOTCH_Num));
      rc = SCOTCH_graphPart (&grafdat, 0, &stratdat, ptab);
      /* Property: doesn't crash. Error return expected. */
      (void) rc;

      free (ptab);
      SCOTCH_stratExit (&stratdat);
      SCOTCH_graphExit (&grafdat);
      free (verttab);
      free (edgetab);
      break;
    }
    case 3: {
      /* stratGraphMap with empty string: should use default */
      HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
      rc = SCOTCH_stratGraphMap (&stratdat, "");
      /* Property: doesn't crash */
      (void) rc;
      SCOTCH_stratExit (&stratdat);
      break;
    }
    case 4: {
      /* graphColor on empty graph */
      verttab_empty[0] = 0;
      edgetab_dummy[0] = 0;

      HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
      rc = SCOTCH_graphBuild (&grafdat, 0, 0,
                              verttab_empty, NULL, NULL, NULL,
                              0, edgetab_dummy, NULL);
      if (rc == 0) {
        SCOTCH_Num          colonbr;

        colonbr = 0;
        rc = SCOTCH_graphColor (&grafdat, NULL, &colonbr, 0);
        /* Property: doesn't crash on empty graph */
        (void) rc;
      }
      SCOTCH_graphExit (&grafdat);
      break;
    }
  }
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running error path tests...\n");
  hegel_run_test (testErrorGraphBuild);
  printf ("PASSED\n");

  return (0);
}
