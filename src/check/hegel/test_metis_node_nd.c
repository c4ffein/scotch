#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "metis.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: METIS_NodeND produces a valid permutation.
** The perm and iperm arrays must be inverses of each other.
*/
static
void
testMetisNodeND (
hegel_testcase *            tc)
{
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          numflag;
  SCOTCH_Num          optval;
  SCOTCH_Num *        permtab;
  SCOTCH_Num *        ipermtab;
  SCOTCH_Num          vertnum;
  char *              seen;
  int                 rc;

  /* Use baseval 0 to avoid the known base-1 bug in MeTiS compat */
  graphGenRandom (tc, 0, &vertnbr, &verttab, &edgetab, &edgenbr);
  hegel_assume (tc, edgenbr > 0);

  baseval = 0;
  numflag = 0;
  optval  = 0;

  permtab  = malloc (vertnbr * sizeof (SCOTCH_Num));
  ipermtab = malloc (vertnbr * sizeof (SCOTCH_Num));

  rc = METIS_NodeND (&vertnbr, verttab, edgetab, &numflag, &optval, permtab, ipermtab);
  HEGEL_ASSERT (rc == METIS_OK, "METIS_NodeND failed: %d", rc);

  /* Property: permtab is a valid permutation of [0, vertnbr) */
  seen = calloc (vertnbr, sizeof (char));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (permtab[vertnum] >= 0 && permtab[vertnum] < vertnbr,
                  "permtab[%d] = %d out of range",
                  (int) vertnum, (int) permtab[vertnum]);
    HEGEL_ASSERT (!seen[permtab[vertnum]],
                  "permtab duplicate: %d", (int) permtab[vertnum]);
    seen[permtab[vertnum]] = 1;
  }

  /* Property: ipermtab is inverse of permtab */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (ipermtab[permtab[vertnum]] == vertnum,
                  "ipermtab[permtab[%d]] = %d, expected %d",
                  (int) vertnum, (int) ipermtab[permtab[vertnum]], (int) vertnum);
  }

  free (seen);
  free (ipermtab);
  free (permtab);
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

  printf ("Running METIS_NodeND property test...\n");
  hegel_run_test (testMetisNodeND);
  printf ("PASSED\n");

  return (0);
}
