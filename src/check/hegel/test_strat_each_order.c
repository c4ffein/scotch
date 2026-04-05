#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"
#include "scotch_helpers.h"

/*
** Property: each individual ordering method produces valid permutations.
** Methods: n (nested dissection), d (approx min degree), s (simple)
*/

static const char * const order_strats[] = {
  "n{sep=m{vert=100,low=h{pass=10},asc=b{width=3,bnd=f{bal=0.05},org=h{pass=10}}},ole=s,ose=s}",
  "n{sep=h{pass=10},ole=s,ose=s}",
  "n{sep=f{bal=0.05,move=200},ole=s,ose=s}",
  "d{cmin=15,cmax=100000,frat=0.08}",
  "s",
};
#define ORDER_NBRSTRATS (sizeof (order_strats) / sizeof (order_strats[0]))

static
void
testStratEachOrder (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        permtab;
  SCOTCH_Num          cblknbr;
  SCOTCH_Num          vertnum;
  char *              seen;
  int                 stratidx;

  scotchReset ();
  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  stratidx = hegel_draw_int (tc, 0, ORDER_NBRSTRATS - 1);

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");
  HEGEL_ASSERT (SCOTCH_stratGraphOrder (&stratdat, order_strats[stratidx]) == 0,
                "stratGraphOrder('%s') failed", order_strats[stratidx]);

  permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  cblknbr = 0;
  HEGEL_ASSERT (SCOTCH_graphOrder (&grafdat, &stratdat,
                                   permtab, NULL, &cblknbr,
                                   NULL, NULL) == 0,
                "graphOrder with '%s' failed", order_strats[stratidx]);

  seen = calloc (vertnbr, sizeof (char));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          p;

    p = permtab[vertnum] - baseval;
    HEGEL_ASSERT (p >= 0 && p < vertnbr,
                  "permtab[%d] = %d out of range (order method '%s')",
                  (int) vertnum, (int) permtab[vertnum], order_strats[stratidx]);
    HEGEL_ASSERT (!seen[p],
                  "permtab duplicate %d (order method '%s')",
                  (int) permtab[vertnum], order_strats[stratidx]);
    seen[p] = 1;
  }

  free (seen);
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

  printf ("Running each ordering method test...\n");
  hegel_run_test (testStratEachOrder);
  printf ("PASSED\n");

  return (0);
}
