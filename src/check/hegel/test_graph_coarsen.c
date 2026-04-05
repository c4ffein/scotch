#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

static
void
buildRandomGraph (
hegel_testcase *            tc,
SCOTCH_Num *                vertnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          vertnbr;
  SCOTCH_Num          nedges;
  SCOTCH_Num          maxedges;
  SCOTCH_Num **       adj;
  SCOTCH_Num *        adjsiz;
  SCOTCH_Num *        adjnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  vertnbr = hegel_draw_int (tc, 8, 50);  /* Need enough vertices for coarsening */
  maxedges = vertnbr * 3;
  nedges = hegel_draw_int (tc, vertnbr, maxedges);  /* Ensure some edges for matching */

  adj    = malloc (vertnbr * sizeof (SCOTCH_Num *));
  adjsiz = calloc (vertnbr, sizeof (SCOTCH_Num));
  adjnbr = calloc (vertnbr, sizeof (SCOTCH_Num));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    adjsiz[vertnum] = 8;
    adj[vertnum] = malloc (adjsiz[vertnum] * sizeof (SCOTCH_Num));
  }

  for (edgenum = 0; edgenum < nedges; edgenum ++) {
    SCOTCH_Num          u, v;
    int                 dup;
    SCOTCH_Num          i;

    u = hegel_draw_int (tc, 0, vertnbr - 1);
    v = hegel_draw_int (tc, 0, vertnbr - 1);
    if (u == v)
      continue;

    dup = 0;
    for (i = 0; i < adjnbr[u]; i ++) {
      if (adj[u][i] == v) {
        dup = 1;
        break;
      }
    }
    if (dup)
      continue;

    if (adjnbr[u] >= adjsiz[u]) {
      adjsiz[u] *= 2;
      adj[u] = realloc (adj[u], adjsiz[u] * sizeof (SCOTCH_Num));
    }
    adj[u][adjnbr[u] ++] = v;

    if (adjnbr[v] >= adjsiz[v]) {
      adjsiz[v] *= 2;
      adj[v] = realloc (adj[v], adjsiz[v] * sizeof (SCOTCH_Num));
    }
    adj[v][adjnbr[v] ++] = u;
  }

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          i, j;

    for (i = 1; i < adjnbr[vertnum]; i ++) {
      SCOTCH_Num          key;

      key = adj[vertnum][i];
      j = i - 1;
      while (j >= 0 && adj[vertnum][j] > key) {
        adj[vertnum][j + 1] = adj[vertnum][j];
        j --;
      }
      adj[vertnum][j + 1] = key;
    }
  }

  edgenbr = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++)
    edgenbr += adjnbr[vertnum];

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));

  edgenbr = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          i;

    verttab[vertnum] = edgenbr;
    for (i = 0; i < adjnbr[vertnum]; i ++)
      edgetab[edgenbr ++] = adj[vertnum][i];
  }
  verttab[vertnbr] = edgenbr;

  for (vertnum = 0; vertnum < vertnbr; vertnum ++)
    free (adj[vertnum]);
  free (adj);
  free (adjsiz);
  free (adjnbr);

  *vertnbrptr = vertnbr;
  *verttabptr = verttab;
  *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Property: coarsening produces a valid coarse graph
** with fewer or equal vertices.
** graphCoarsen returns 0 (success), 1 (threshold not met), or 2 (error).
*/
static
void
testCoarsen (
hegel_testcase *            tc)
{
  SCOTCH_Graph        finegrafdat;
  SCOTCH_Graph        coargrafdat;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        coarmulttab;
  SCOTCH_Num          coarvertnbr;
  int                 rc;

  buildRandomGraph (tc, &vertnbr, &verttab, &edgetab, &edgenbr);

  HEGEL_ASSERT (SCOTCH_graphInit (&finegrafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&finegrafdat, 0, vertnbr,
                                   verttab, NULL, NULL, NULL,
                                   edgenbr, edgetab, NULL) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&finegrafdat) == 0, "graphCheck failed");

  /* Allocate multinode array (worst case: vertnbr coarse vertices) */
  coarmulttab = malloc (vertnbr * 2 * sizeof (SCOTCH_Num));

  /* coarsenval = 1 (min coarse vertices), ratio = 0.8 */
  rc = SCOTCH_graphCoarsen (&finegrafdat, 1, 0.8, SCOTCH_COARSENNONE,
                            &coargrafdat, coarmulttab);

  /* rc=2 is error, rc=0 is success, rc=1 is threshold not met (not error) */
  HEGEL_ASSERT (rc != 2, "graphCoarsen returned error (2)");

  if (rc == 0) {
    /* Property: coarse graph passes graphCheck */
    HEGEL_ASSERT (SCOTCH_graphCheck (&coargrafdat) == 0,
                  "graphCheck on coarse graph failed");

    /* Property: coarse graph has fewer or equal vertices */
    SCOTCH_graphSize (&coargrafdat, &coarvertnbr, NULL);
    HEGEL_ASSERT (coarvertnbr <= vertnbr,
                  "coarse vertnbr %d > fine vertnbr %d",
                  (int) coarvertnbr, (int) vertnbr);

    SCOTCH_graphExit (&coargrafdat);
  }

  free (coarmulttab);
  SCOTCH_graphExit (&finegrafdat);
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

  printf ("Running graphCoarsen property test...\n");
  hegel_run_test (testCoarsen);
  printf ("PASSED\n");

  return (0);
}
