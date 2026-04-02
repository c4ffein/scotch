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
** Property: inducing a subgraph from a random vertex subset
** produces a valid graph with fewer or equal vertices/edges.
*/
static
void
testInduce (
hegel_testcase *            tc)
{
  SCOTCH_Graph        orggrafdat;
  SCOTCH_Graph        indgrafdat;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          indvertnbr;
  SCOTCH_Num *        indlisttab;
  SCOTCH_Num          indvertnbr_out;
  SCOTCH_Num          indedgenbr_out;
  char *              chosen;

  buildRandomGraph (tc, &vertnbr, &verttab, &edgetab, &edgenbr);

  HEGEL_ASSERT (SCOTCH_graphInit (&orggrafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&orggrafdat, 0, vertnbr,
                                   verttab, NULL, NULL, NULL,
                                   edgenbr, edgetab, NULL) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&orggrafdat) == 0, "graphCheck failed");

  /* Choose a random subset of vertices (at least 1) */
  indvertnbr = hegel_draw_int (tc, 1, vertnbr);
  indlisttab = malloc (indvertnbr * sizeof (SCOTCH_Num));
  chosen = calloc (vertnbr, sizeof (char));

  /* Pick indvertnbr distinct vertices */
  {
    SCOTCH_Num          count;

    count = 0;
    while (count < indvertnbr) {
      SCOTCH_Num          v;

      v = hegel_draw_int (tc, 0, vertnbr - 1);
      if (!chosen[v]) {
        chosen[v] = 1;
        indlisttab[count ++] = v;
      }
    }
  }

  HEGEL_ASSERT (SCOTCH_graphInit (&indgrafdat) == 0, "graphInit (ind) failed");
  HEGEL_ASSERT (SCOTCH_graphInduceList (&orggrafdat, indvertnbr, indlisttab, &indgrafdat) == 0,
                "graphInduceList failed");

  /* Property: induced graph passes graphCheck */
  HEGEL_ASSERT (SCOTCH_graphCheck (&indgrafdat) == 0, "graphCheck on induced graph failed");

  /* Property: induced graph has exactly indvertnbr vertices */
  SCOTCH_graphSize (&indgrafdat, &indvertnbr_out, &indedgenbr_out);
  HEGEL_ASSERT (indvertnbr_out == indvertnbr,
                "induced vertnbr: expected %d, got %d",
                (int) indvertnbr, (int) indvertnbr_out);

  /* Property: induced graph has <= original edges */
  HEGEL_ASSERT (indedgenbr_out <= edgenbr,
                "induced edgenbr %d > original %d",
                (int) indedgenbr_out, (int) edgenbr);

  SCOTCH_graphExit (&indgrafdat);
  free (chosen);
  free (indlisttab);
  SCOTCH_graphExit (&orggrafdat);
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

  printf ("Running graphInduceList property test...\n");
  hegel_run_test (testInduce);
  printf ("PASSED\n");

  return (0);
}
