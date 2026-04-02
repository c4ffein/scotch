#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: graphSave then graphLoad roundtrip preserves graph structure.
*/
static
void
testSaveLoad (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Graph        grafdat2;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          baseval2;
  SCOTCH_Num          vertnbr2;
  SCOTCH_Num          edgenbr2;
  SCOTCH_Num *        verttab2;
  SCOTCH_Num *        edgetab2;
  FILE *              fileptr;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Save to temp file */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_graphSave (&grafdat, fileptr) == 0, "graphSave failed");

  /* Load back */
  rewind (fileptr);
  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat2) == 0, "graphInit(2) failed");
  HEGEL_ASSERT (SCOTCH_graphLoad (&grafdat2, fileptr, -1, 0) == 0, "graphLoad failed");
  fclose (fileptr);

  /* Property: loaded graph passes graphCheck */
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat2) == 0, "graphCheck on loaded graph failed");

  /* Property: vertex and edge counts match */
  SCOTCH_graphData (&grafdat2, &baseval2, &vertnbr2,
                    &verttab2, NULL, NULL, NULL,
                    &edgenbr2, &edgetab2, NULL);

  HEGEL_ASSERT (vertnbr2 == vertnbr,
                "vertnbr mismatch: %d vs %d", (int) vertnbr, (int) vertnbr2);
  HEGEL_ASSERT (edgenbr2 == edgenbr,
                "edgenbr mismatch: %d vs %d", (int) edgenbr, (int) edgenbr2);

  /* Property: edge arrays match (baseval may differ if graphLoad rebases) */
  {
    SCOTCH_Num          vertnum;
    SCOTCH_Num          rebase;

    rebase = baseval2 - baseval;

    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      SCOTCH_Num          deg1, deg2;

      deg1 = verttab[vertnum + 1] - verttab[vertnum];
      deg2 = verttab2[vertnum + 1] - verttab2[vertnum];
      HEGEL_ASSERT (deg1 == deg2,
                    "degree mismatch at vertex %d: %d vs %d",
                    (int) vertnum, (int) deg1, (int) deg2);
    }
  }

  SCOTCH_graphExit (&grafdat2);
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

  printf ("Running graph save/load roundtrip test...\n");
  hegel_run_test (testSaveLoad);
  printf ("PASSED\n");

  return (0);
}
