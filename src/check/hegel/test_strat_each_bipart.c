#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"
#include "scotch_helpers.h"

/*
** Property: each individual bipartitioning method produces valid
** partitions when used as the separator in a recursive mapping strategy.
** Methods: f (FM), h (greedy grow), g (greedy part), m (multi-level), z (zero)
*/

static const char * const bipart_strats[] = {
  "r{sep=f{bal=0.05,move=200}}",                  /* FM refinement */
  "r{sep=h{pass=10}}",                            /* Greedy growing (GG) */
  "r{sep=g{pass=5}}",                             /* Greedy partitioning (GP) */
  "r{sep=m{vert=100,low=h{pass=10},asc=f{bal=0.05}}}", /* Multi-level */
  "r{sep=z}",                                     /* Zero (all to part 0) */
  "r{sep=b{width=3,bnd=f{bal=0.05},org=h{pass=10}}}", /* Band */
};
#define BIPART_NBRSTRATS (sizeof (bipart_strats) / sizeof (bipart_strats[0]))

static
void
testStratEachBipart (
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
  SCOTCH_Num          vertnum;
  int                 stratidx;

  scotchReset ();
  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  nparts   = hegel_draw_int (tc, 2, 4);
  stratidx = hegel_draw_int (tc, 0, BIPART_NBRSTRATS - 1);

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_stratGraphMap (&stratdat, bipart_strats[stratidx]) == 0,
                "stratGraphMap('%s') failed", bipart_strats[stratidx]);

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphPart (&grafdat, nparts, &stratdat, parttab) == 0,
                "graphPart with '%s' failed", bipart_strats[stratidx]);

  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < nparts,
                  "vertex %d: part=%d (bipart method '%s')",
                  (int) vertnum, (int) parttab[vertnum], bipart_strats[stratidx]);
  }

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

  printf ("Running each bipartitioning method test...\n");
  hegel_run_test (testStratEachBipart);
  printf ("PASSED\n");

  return (0);
}
