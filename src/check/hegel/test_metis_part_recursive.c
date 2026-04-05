#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "metis.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: METIS_PartGraphRecursive (v3 API) with baseval=0
** produces partition values in [0, nparts) and returns METIS_OK.
** Signature (v3): METIS_PartGraphRecursive(n, xadj, adjncy,
**   vwgt, adjwgt, wgtflag, numflag, nparts, options, edgecut, part)
*/
static
void
testMetisPartRecursive (
hegel_testcase *            tc)
{
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

  /* Generate a base-0 graph for clean MeTiS semantics */
  graphGenRandom (tc, 0, &vertnbr, &verttab, &edgetab, &edgenbr);

  /* Need at least some edges for MeTiS */
  hegel_assume (tc, edgenbr > 0);

  numflag = 0;                                      /* C-style numbering */
  nparts  = hegel_draw_int (tc, 2, 8);
  wgtflag = 0;                                      /* No weights */
  optval  = 0;                                      /* Default options */

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));

  rc = METIS_PartGraphRecursive (&vertnbr, verttab, edgetab, NULL, NULL,
                                 &wgtflag, &numflag, &nparts, &optval,
                                 &edgecut, parttab);

  HEGEL_ASSERT (rc == METIS_OK, "METIS_PartGraphRecursive failed: %d", rc);

  /* Property: all partition values must be in [0, nparts) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) nparts);
  }

  /* Property: edgecut is non-negative */
  HEGEL_ASSERT (edgecut >= 0,
                "edgecut is negative: %d", (int) edgecut);

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

  printf ("Running MeTiS PartGraphRecursive property test...\n");
  hegel_run_test (testMetisPartRecursive);
  printf ("PASSED\n");

  return (0);
}
