#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: SCOTCH_graphOrderList orders a subset of vertices
** and produces a valid permutation for those vertices.
** orderCheck passes on the result.
*/
static
void
testOrderList (
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
  SCOTCH_Num          listnbr;
  SCOTCH_Num *        listtab;
  SCOTCH_Num          listnum;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Build a random subset of vertices */
  listnbr = hegel_draw_int (tc, 1, vertnbr);
  listtab = malloc (listnbr * sizeof (SCOTCH_Num));
  {
    char *              used;
    SCOTCH_Num          count;

    used  = calloc (vertnbr, sizeof (char));
    count = 0;
    while (count < listnbr) {
      SCOTCH_Num          v;

      v = hegel_draw_int (tc, 0, vertnbr - 1);
      if (!used[v]) {
        used[v] = 1;
        listtab[count ++] = v + baseval;
      }
    }
    free (used);
  }

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  peritab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;

  HEGEL_ASSERT (SCOTCH_graphOrderList (&grafdat, listnbr, listtab,
                                       &stratdat, permtab, peritab,
                                       &cblknbr, NULL, NULL) == 0,
                "graphOrderList failed");

  /* Check that listed vertices have valid permutation values in [baseval, baseval + listnbr) */
  {
    char *              seen;

    seen = calloc (listnbr, sizeof (char));
    for (listnum = 0; listnum < listnbr; listnum ++) {
      SCOTCH_Num          v;
      SCOTCH_Num          p;

      v = listtab[listnum] - baseval;   /* 0-based vertex index */
      p = permtab[v];
      HEGEL_ASSERT (p >= baseval && p < baseval + listnbr,
                    "permtab[%d] = %d out of range [%d, %d)",
                    (int) v, (int) p,
                    (int) baseval, (int) (baseval + listnbr));
      HEGEL_ASSERT (!seen[p - baseval],
                    "permtab has duplicate value %d", (int) p);
      seen[p - baseval] = 1;
    }
    free (seen);
  }

  /* Use Scotch's own orderCheck via the low-level API */
  HEGEL_ASSERT (SCOTCH_graphOrderInit (&grafdat, &ordedat,
                                       permtab, peritab, &cblknbr,
                                       NULL, NULL) == 0,
                "graphOrderInit failed");
  HEGEL_ASSERT (SCOTCH_graphOrderComputeList (&grafdat, &ordedat,
                                              listnbr, listtab,
                                              &stratdat) == 0,
                "graphOrderComputeList failed");
  HEGEL_ASSERT (SCOTCH_graphOrderCheck (&grafdat, &ordedat) == 0,
                "graphOrderCheck failed");
  SCOTCH_graphOrderExit (&grafdat, &ordedat);

  free (listtab);
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

  printf ("Running graphOrderList property test...\n");
  hegel_run_test (testOrderList);
  printf ("PASSED\n");

  return (0);
}
