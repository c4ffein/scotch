#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: partitioning with fixed vertices respects the fixed assignments.
** Fixed vertices (parttab[v] >= 0 before call) must keep their partition.
** Free vertices (parttab[v] == -1 before call) get assigned [0, nparts).
*/
static
void
testPartFixed (
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
  SCOTCH_Num *        fixedcopy;
  SCOTCH_Num          vertnum;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts = hegel_draw_int (tc, 2, 4);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab   = malloc (vertnbr * sizeof (SCOTCH_Num));
  fixedcopy = malloc (vertnbr * sizeof (SCOTCH_Num));

  /* Set some vertices as fixed, others as free (-1) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    if (hegel_draw_int (tc, 0, 3) == 0)           /* ~25% fixed */
      parttab[vertnum] = hegel_draw_int (tc, 0, nparts - 1);
    else
      parttab[vertnum] = -1;
  }
  memcpy (fixedcopy, parttab, vertnbr * sizeof (SCOTCH_Num));

  HEGEL_ASSERT (SCOTCH_graphPartFixed (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPartFixed failed");

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    /* All vertices must have valid partition */
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);

    /* Fixed vertices must keep their assignment */
    if (fixedcopy[vertnum] >= 0) {
      HEGEL_ASSERT (parttab[vertnum] == fixedcopy[vertnum],
                    "fixed vertex %d changed: %d -> %d",
                    (int) vertnum, (int) fixedcopy[vertnum], (int) parttab[vertnum]);
    }
  }

  free (fixedcopy);
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

  printf ("Running fixed partition property test...\n");
  hegel_run_test (testPartFixed);
  printf ("PASSED\n");

  return (0);
}
