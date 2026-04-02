#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Build a random undirected graph in CSR format.
** Caller must free verttab and edgetab.
*/
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

  vertnbr = hegel_draw_int (tc, 4, 50);
  maxedges = vertnbr * 2;
  nedges = hegel_draw_int (tc, 0, maxedges);

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
** Property: graphBuild + graphData roundtrip.
** Arrays extracted via graphData must match those passed to graphBuild.
*/
static
void
testBuildData (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          baseval_out;
  SCOTCH_Num          vertnbr_out;
  SCOTCH_Num          edgenbr_out;
  SCOTCH_Num *        verttab_out;
  SCOTCH_Num *        edgetab_out;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  buildRandomGraph (tc, &vertnbr, &verttab, &edgetab, &edgenbr);

  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&grafdat, 0, vertnbr,
                                   verttab, NULL, NULL, NULL,
                                   edgenbr, edgetab, NULL) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0, "graphCheck failed");

  /* Extract data back */
  verttab_out = NULL;
  edgetab_out = NULL;
  SCOTCH_graphData (&grafdat, &baseval_out, &vertnbr_out,
                    &verttab_out, NULL, NULL, NULL,
                    &edgenbr_out, &edgetab_out, NULL);

  HEGEL_ASSERT (baseval_out == 0, "baseval mismatch: got %d", (int) baseval_out);
  HEGEL_ASSERT (vertnbr_out == vertnbr,
                "vertnbr mismatch: expected %d, got %d",
                (int) vertnbr, (int) vertnbr_out);
  HEGEL_ASSERT (edgenbr_out == edgenbr,
                "edgenbr mismatch: expected %d, got %d",
                (int) edgenbr, (int) edgenbr_out);

  /* verttab should match (graphBuild with NULL vendtab uses contiguous storage) */
  for (vertnum = 0; vertnum <= vertnbr; vertnum ++) {
    HEGEL_ASSERT (verttab_out[vertnum] == verttab[vertnum],
                  "verttab[%d] mismatch: expected %d, got %d",
                  (int) vertnum, (int) verttab[vertnum], (int) verttab_out[vertnum]);
  }

  /* edgetab should match */
  for (edgenum = 0; edgenum < edgenbr; edgenum ++) {
    HEGEL_ASSERT (edgetab_out[edgenum] == edgetab[edgenum],
                  "edgetab[%d] mismatch: expected %d, got %d",
                  (int) edgenum, (int) edgetab[edgenum], (int) edgetab_out[edgenum]);
  }

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

  printf ("Running graphBuild+graphData roundtrip test...\n");
  hegel_run_test (testBuildData);
  printf ("PASSED\n");

  return (0);
}
