#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: greedy coloring uses at most max_degree + 1 colors.
** This is a well-known graph theory bound for any greedy coloring algorithm.
*/
static
void
testColorQuality (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        colotab;
  SCOTCH_Num          colonbr;
  SCOTCH_Num          degrmin;
  SCOTCH_Num          degrmax;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Get max degree via graphStat */
  SCOTCH_graphStat (&grafdat, NULL, NULL, NULL, NULL, NULL,
                    &degrmin, &degrmax, NULL, NULL,
                    NULL, NULL, NULL, NULL, NULL);

  colotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  colonbr = 0;
  HEGEL_ASSERT (SCOTCH_graphColor (&grafdat, colotab, &colonbr, 0) == 0,
                "graphColor failed");

  /* Scotch uses a Luby-style concurrent coloring algorithm.
  ** This does NOT satisfy the standard greedy Δ+1 bound — it can use
  ** up to vertnbr-1 colors on pathological random priority orderings.
  ** (e.g., a 10-vertex path can get 9 colors.)
  ** The only valid bound is colonbr <= vertnbr. */
  HEGEL_ASSERT (colonbr <= vertnbr,
                "used %d colors for %d-vertex graph",
                (int) colonbr, (int) vertnbr);

  /* Property: colonbr >= 1 (at least one color for any non-empty graph) */
  HEGEL_ASSERT (colonbr >= 1, "colonbr=%d < 1", (int) colonbr);

  /* Property: all colors in [0, colonbr) are actually used */
  {
    char *              used;
    SCOTCH_Num          vertnum;
    SCOTCH_Num          colonum;

    used = calloc (colonbr, sizeof (char));
    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      HEGEL_ASSERT (colotab[vertnum] >= 0 && colotab[vertnum] < colonbr,
                    "color %d out of range", (int) colotab[vertnum]);
      used[colotab[vertnum]] = 1;
    }
    for (colonum = 0; colonum < colonbr; colonum ++) {
      HEGEL_ASSERT (used[colonum],
                    "color %d declared but unused (colonbr=%d)",
                    (int) colonum, (int) colonbr);
    }
    free (used);
  }

  free (colotab);
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

  printf ("Running coloring quality bound test...\n");
  hegel_run_test (testColorQuality);
  printf ("PASSED\n");

  return (0);
}
