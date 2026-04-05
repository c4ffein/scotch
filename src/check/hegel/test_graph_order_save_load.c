#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: compute an ordering, save it with graphOrderSave,
** reload with graphOrderLoad, and the loaded ordering passes orderCheck.
*/
static
void
testOrderSaveLoad (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Ordering     ordedat;
  SCOTCH_Ordering     ordedat2;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num *        peritab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num *        permtab2;
  SCOTCH_Num *        peritab2;
  SCOTCH_Num          cblknbr2;
  FILE *              fileptr;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  /* Compute ordering */
  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  peritab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;

  HEGEL_ASSERT (SCOTCH_graphOrderInit (&grafdat, &ordedat,
                                       permtab, peritab, &cblknbr,
                                       NULL, NULL) == 0,
                "graphOrderInit failed");
  HEGEL_ASSERT (SCOTCH_graphOrderCompute (&grafdat, &ordedat, &stratdat) == 0,
                "graphOrderCompute failed");
  HEGEL_ASSERT (SCOTCH_graphOrderCheck (&grafdat, &ordedat) == 0,
                "graphOrderCheck failed on original ordering");

  /* Save ordering to temp file */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_graphOrderSave (&grafdat, &ordedat, fileptr) == 0,
                "graphOrderSave failed");

  SCOTCH_graphOrderExit (&grafdat, &ordedat);

  /* Load ordering back */
  rewind (fileptr);

  permtab2 = malloc (vertnbr * sizeof (SCOTCH_Num));
  peritab2 = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr2 = 0;

  HEGEL_ASSERT (SCOTCH_graphOrderInit (&grafdat, &ordedat2,
                                       permtab2, peritab2, &cblknbr2,
                                       NULL, NULL) == 0,
                "graphOrderInit(2) failed");
  HEGEL_ASSERT (SCOTCH_graphOrderLoad (&grafdat, &ordedat2, fileptr) == 0,
                "graphOrderLoad failed");
  fclose (fileptr);

  /* Property: loaded ordering passes orderCheck */
  HEGEL_ASSERT (SCOTCH_graphOrderCheck (&grafdat, &ordedat2) == 0,
                "graphOrderCheck failed on loaded ordering");

  /* Property: loaded permtab matches original */
  {
    SCOTCH_Num          vertnum;

    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      HEGEL_ASSERT (permtab2[vertnum] == permtab[vertnum],
                    "permtab mismatch at vertex %d: %d vs %d",
                    (int) vertnum, (int) permtab2[vertnum], (int) permtab[vertnum]);
    }
  }

  SCOTCH_graphOrderExit (&grafdat, &ordedat2);

  free (peritab2);
  free (permtab2);
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

  printf ("Running ordering save/load roundtrip test...\n");
  hegel_run_test (testOrderSaveLoad);
  printf ("PASSED\n");

  return (0);
}
