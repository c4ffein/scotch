#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: save a partition array with graphTabSave, reload with
** graphTabLoad. The loaded array matches the original.
*/
static
void
testTabSaveLoad (
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
  SCOTCH_Num *        parttab2;
  SCOTCH_Num          vertnum;
  FILE *              fileptr;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Compute a partition */
  nparts = hegel_draw_int (tc, 2, 8);
  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart failed");

  /* Save partition to temp file */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_graphTabSave (&grafdat, parttab, fileptr) == 0,
                "graphTabSave failed");

  /* Load partition back */
  rewind (fileptr);
  parttab2 = malloc (vertnbr * sizeof (SCOTCH_Num));
  memset (parttab2, -1, vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphTabLoad (&grafdat, parttab2, fileptr) == 0,
                "graphTabLoad failed");
  fclose (fileptr);

  /* Property: loaded array matches original */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab2[vertnum] == parttab[vertnum],
                  "parttab mismatch at vertex %d: loaded %d vs original %d",
                  (int) vertnum, (int) parttab2[vertnum], (int) parttab[vertnum]);
  }

  free (parttab2);
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

  printf ("Running graphTabSave/graphTabLoad roundtrip test...\n");
  hegel_run_test (testTabSaveLoad);
  printf ("PASSED\n");

  return (0);
}
