#ifndef GRAPH_GEN_H
#define GRAPH_GEN_H

#include <stdlib.h>
#include <string.h>
#include "scotch.h"
#include "hegel_c.h"

/*
** When compiled with -DHEGEL_BENCH_NOFORK, redirect hegel_run_test
** to the nofork variant for benchmarking fork overhead.
*/
#ifdef HEGEL_BENCH_NOFORK
#define hegel_run_test   hegel_run_test_nofork
#define hegel_run_test_n hegel_run_test_nofork_n
#endif

/*
** Reset Scotch's global random state for deterministic behavior.
*/
static
void
scotchResetForHegel_ (void)
{
  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
}

/*
** Register Scotch's RNG reset as a per-test-case setup callback.
** Uses __attribute__((constructor)) so it runs automatically before main().
** This keeps hegel-c itself free of Scotch dependencies.
*/
__attribute__((constructor))
static
void
scotchSetupHegel_ (void)
{
  hegel_set_case_setup (scotchResetForHegel_);
}


/*
** Graph generation utilities for Hegel PBT tests.
** Each generator builds a valid graph in CSR format (verttab/edgetab)
** with the specified baseval.
** Caller must free verttab and edgetab.
*/

/* Graph type enum for random selection */
#define GRAPHGEN_RANDOM   0
#define GRAPHGEN_GRID2D   1
#define GRAPHGEN_COMPLETE 2
#define GRAPHGEN_PATH     3
#define GRAPHGEN_NBTYPES  4

/*
** Build a random sparse undirected graph.
*/
static
void
graphGenRandom (
hegel_testcase *            tc,
const SCOTCH_Num            baseval,
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
      if (adj[u][i] == v + baseval) {
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
    adj[u][adjnbr[u] ++] = v + baseval;

    if (adjnbr[v] >= adjsiz[v]) {
      adjsiz[v] *= 2;
      adj[v] = realloc (adj[v], adjsiz[v] * sizeof (SCOTCH_Num));
    }
    adj[v][adjnbr[v] ++] = u + baseval;
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

    verttab[vertnum] = edgenbr + baseval;
    for (i = 0; i < adjnbr[vertnum]; i ++)
      edgetab[edgenbr ++] = adj[vertnum][i];
  }
  verttab[vertnbr] = edgenbr + baseval;

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
** Build a 2D grid graph (dim0 x dim1).
*/
static
void
graphGenGrid2D (
hegel_testcase *            tc,
const SCOTCH_Num            baseval,
SCOTCH_Num *                vertnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          dim0, dim1;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  dim0 = hegel_draw_int (tc, 2, 8);
  dim1 = hegel_draw_int (tc, 2, 8);
  vertnbr = dim0 * dim1;

  /* Count edges: each interior edge counted twice (undirected) */
  edgenbr = 2 * ((dim0 - 1) * dim1 + dim0 * (dim1 - 1));

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          x, y;

    x = vertnum % dim0;
    y = vertnum / dim0;

    verttab[vertnum] = edgenum + baseval;

    if (x > 0)
      edgetab[edgenum ++] = (y * dim0 + (x - 1)) + baseval;
    if (x < dim0 - 1)
      edgetab[edgenum ++] = (y * dim0 + (x + 1)) + baseval;
    if (y > 0)
      edgetab[edgenum ++] = ((y - 1) * dim0 + x) + baseval;
    if (y < dim1 - 1)
      edgetab[edgenum ++] = ((y + 1) * dim0 + x) + baseval;
  }
  verttab[vertnbr] = edgenum + baseval;
  edgenbr = edgenum;

  *vertnbrptr = vertnbr;
  *verttabptr = verttab;
  *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Build a complete graph (K_n).
*/
static
void
graphGenComplete (
hegel_testcase *            tc,
const SCOTCH_Num            baseval,
SCOTCH_Num *                vertnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  vertnbr = hegel_draw_int (tc, 4, 16);  /* Keep small — K_n has n*(n-1) arcs */
  edgenbr = vertnbr * (vertnbr - 1);

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          vertend;

    verttab[vertnum] = edgenum + baseval;
    for (vertend = 0; vertend < vertnbr; vertend ++) {
      if (vertend != vertnum)
        edgetab[edgenum ++] = vertend + baseval;
    }
  }
  verttab[vertnbr] = edgenum + baseval;

  *vertnbrptr = vertnbr;
  *verttabptr = verttab;
  *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Build a path graph (P_n).
*/
static
void
graphGenPath (
hegel_testcase *            tc,
const SCOTCH_Num            baseval,
SCOTCH_Num *                vertnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          vertnum;
  SCOTCH_Num          edgenum;

  vertnbr = hegel_draw_int (tc, 4, 50);
  edgenbr = 2 * (vertnbr - 1);

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));

  edgenum = 0;
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    verttab[vertnum] = edgenum + baseval;
    if (vertnum > 0)
      edgetab[edgenum ++] = (vertnum - 1) + baseval;
    if (vertnum < vertnbr - 1)
      edgetab[edgenum ++] = (vertnum + 1) + baseval;
  }
  verttab[vertnbr] = edgenum + baseval;

  *vertnbrptr = vertnbr;
  *verttabptr = verttab;
  *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Build a random graph of a random type, with a random baseval.
*/
static
void
graphGenAny (
hegel_testcase *            tc,
SCOTCH_Num *                basevalptr,
SCOTCH_Num *                vertnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          baseval;
  int                 graphtype;

  baseval   = hegel_draw_int (tc, 0, 1);  /* 0 (C) or 1 (Fortran) */
  graphtype = hegel_draw_int (tc, 0, GRAPHGEN_NBTYPES - 1);

  switch (graphtype) {
    case GRAPHGEN_GRID2D:
      graphGenGrid2D (tc, baseval, vertnbrptr, verttabptr, edgetabptr, edgenbrptr);
      break;
    case GRAPHGEN_COMPLETE:
      graphGenComplete (tc, baseval, vertnbrptr, verttabptr, edgetabptr, edgenbrptr);
      break;
    case GRAPHGEN_PATH:
      graphGenPath (tc, baseval, vertnbrptr, verttabptr, edgetabptr, edgenbrptr);
      break;
    default:
      graphGenRandom (tc, baseval, vertnbrptr, verttabptr, edgetabptr, edgenbrptr);
      break;
  }

  *basevalptr = baseval;
}

/*
** Build a SCOTCH_Graph from generated arrays. Returns 0 on success.
** Caller must call SCOTCH_graphExit on the result.
*/
static
int
graphGenBuild (
SCOTCH_Graph *              grafptr,
const SCOTCH_Num            baseval,
const SCOTCH_Num            vertnbr,
const SCOTCH_Num *          verttab,
const SCOTCH_Num *          edgetab,
const SCOTCH_Num            edgenbr)
{
  if (SCOTCH_graphInit (grafptr) != 0)
    return (1);
  if (SCOTCH_graphBuild (grafptr, baseval, vertnbr,
                         verttab, NULL, NULL, NULL,
                         edgenbr, edgetab, NULL) != 0)
    return (1);
  if (SCOTCH_graphCheck (grafptr) != 0)
    return (1);
  return (0);
}

#endif /* GRAPH_GEN_H */
