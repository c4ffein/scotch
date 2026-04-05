#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: graphStat returns consistent statistics.
*/
static
void
testGraphStat (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          velomin;
  SCOTCH_Num          velomax;
  SCOTCH_Num          degrmin;
  SCOTCH_Num          degrmax;
  double              degramean;
  double              degravari;
  SCOTCH_Num          edlomin;
  SCOTCH_Num          edlomax;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  SCOTCH_graphStat (&grafdat,
                    &velomin, &velomax, NULL, NULL, NULL,
                    &degrmin, &degrmax, &degramean, &degravari,
                    &edlomin, &edlomax, NULL, NULL, NULL);

  /* Property: degree min <= degree max */
  HEGEL_ASSERT (degrmin <= degrmax,
                "degrmin %d > degrmax %d", (int) degrmin, (int) degrmax);

  /* Property: degree min >= 0 */
  HEGEL_ASSERT (degrmin >= 0, "degrmin %d < 0", (int) degrmin);

  /* Property: mean degree >= min and <= max */
  HEGEL_ASSERT (degramean >= (double) degrmin - 0.001 &&
                degramean <= (double) degrmax + 0.001,
                "mean degree %f outside [%d, %d]",
                degramean, (int) degrmin, (int) degrmax);

  /* Property: variance >= 0 */
  HEGEL_ASSERT (degravari >= -0.001,
                "degree variance %f < 0", degravari);

  /* Property: for unweighted graph, velomin == velomax == 1 */
  HEGEL_ASSERT (velomin == 1 && velomax == 1,
                "unweighted graph: velomin=%d, velomax=%d, expected 1",
                (int) velomin, (int) velomax);

  /* For grid: max degree <= 4 (2D), <= 2 (path), == vertnbr-1 (complete) */
  /* (not checked — would need to know graph type) */

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

  printf ("Running graphStat property test...\n");
  hegel_run_test (testGraphStat);
  printf ("PASSED\n");

  return (0);
}
