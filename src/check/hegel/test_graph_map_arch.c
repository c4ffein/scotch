#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: graphMap with various architectures produces
** valid mapping values in [0, archSize).
*/
static
void
testMapArch (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Arch         archdat;
  SCOTCH_Strat        stratdat;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        parttab;
  SCOTCH_Num          archsiz;
  SCOTCH_Num          vertnum;
  int                 archtype;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);
  HEGEL_ASSERT (graphGenBuild (&grafdat, baseval, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  HEGEL_ASSERT (SCOTCH_archInit (&archdat) == 0, "archInit failed");

  /* Pick a random fixed-size architecture */
  archtype = hegel_draw_int (tc, 0, 3);

  switch (archtype) {
    case 0: {                                     /* Complete */
      SCOTCH_Num          nparts;

      nparts = hegel_draw_int (tc, 2, 8);
      HEGEL_ASSERT (SCOTCH_archCmplt (&archdat, nparts) == 0,
                    "archCmplt failed");
      break;
    }
    case 1: {                                     /* 2D mesh */
      SCOTCH_Num          d0, d1;

      d0 = hegel_draw_int (tc, 1, 4);
      d1 = hegel_draw_int (tc, 1, 4);
      HEGEL_ASSERT (SCOTCH_archMesh2 (&archdat, d0, d1) == 0,
                    "archMesh2 failed");
      break;
    }
    case 2: {                                     /* 2D torus */
      SCOTCH_Num          d0, d1;

      d0 = hegel_draw_int (tc, 1, 4);
      d1 = hegel_draw_int (tc, 1, 4);
      HEGEL_ASSERT (SCOTCH_archTorus2 (&archdat, d0, d1) == 0,
                    "archTorus2 failed");
      break;
    }
    case 3: {                                     /* Hypercube */
      SCOTCH_Num          dim;

      dim = hegel_draw_int (tc, 1, 4);
      HEGEL_ASSERT (SCOTCH_archHcub (&archdat, dim) == 0,
                    "archHcub failed");
      break;
    }
  }

  archsiz = SCOTCH_archSize (&archdat);
  HEGEL_ASSERT (archsiz > 0, "archSize returned %d", (int) archsiz);

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  parttab = malloc (vertnbr * sizeof (SCOTCH_Num));
  HEGEL_ASSERT (SCOTCH_graphMap (&grafdat, &archdat, &stratdat, parttab) == 0,
                "graphMap failed");

  /* Property: all mapping values in [0, archSize) */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (parttab[vertnum] >= 0 && parttab[vertnum] < archsiz,
                  "vertex %d: map=%d, expected [0, %d)",
                  (int) vertnum, (int) parttab[vertnum], (int) archsiz);
  }

  free (parttab);
  SCOTCH_stratExit (&stratdat);
  SCOTCH_archExit (&archdat);
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

  printf ("Running graphMap architecture property test...\n");
  hegel_run_test (testMapArch);
  printf ("PASSED\n");

  return (0);
}
