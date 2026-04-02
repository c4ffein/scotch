#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "metis.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: METIS_PartGraphKway output must be in [0, nparts).
** Known bug (CLAUDE.md): with numflag != 0 (base-1 graphs),
** the output is in [1, nparts] instead of [0, nparts-1].
** This test should expose that bug.
*/
static
void
testMetisPart (
hegel_testcase *            tc)
{
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          nparts;
  SCOTCH_Num          numflag;
  SCOTCH_Num          wgtflag;
  SCOTCH_Num          optval;
  SCOTCH_Num          edgecut;
  SCOTCH_Num *        parttab;
  SCOTCH_Num          vertnum;
  int                 rc;

  /* Generate a graph — use grid or random, with baseval 0 or 1 */
  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);

  /* Need at least some edges for MeTiS */
  hegel_assume (tc, edgenbr > 0);

  numflag = baseval;
  nparts  = hegel_draw_int (tc, 2, 8);
  wgtflag = 0;                                    /* No weights */
  optval  = 0;                                    /* Default options */

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));

  rc = METIS_PartGraphKway (&vertnbr, verttab, edgetab, NULL, NULL,
                            &wgtflag, &numflag, &nparts, &optval,
                            &edgecut, parttab);

  HEGEL_ASSERT (rc == METIS_OK, "METIS_PartGraphKway failed: %d", rc);

  /* Property: all partition values must be in [0, nparts) */
  /* (MeTiS spec says part is always 0-based, numflag only affects graph indexing) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d, expected [0, %d) (baseval=%d)",
                  (int) vertnum, (int) parttab[vertnum],
                  (int) nparts, (int) baseval);
  }

  free (parttab);
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

  printf ("Running MeTiS partition property test...\n");
  hegel_run_test (testMetisPart);
  printf ("PASSED\n");

  return (0);
}
