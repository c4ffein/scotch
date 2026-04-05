#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: coarsening matching is structurally consistent.
** Every fine edge (u,v) mapping to different coarse vertices (cu,cv)
** implies edge (cu,cv) exists in the coarse graph.
*/
static
void
testCoarsenMatching (
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
  SCOTCH_Num *        coarverttab;
  SCOTCH_Num *        coaredgetab;
  SCOTCH_Num          coarbaseval;
  SCOTCH_Num *        finemap;   /* fine vertex -> coarse vertex mapping */
  SCOTCH_Num          vertnum;
  SCOTCH_Num          coarvertnum;
  int                 rc;

  /* Use base 0 for simpler multinode indexing */
  graphGenGrid2D (tc, 0, &vertnbr, &verttab, &edgetab, &edgenbr);
  hegel_assume (tc, edgenbr > 0);
  hegel_assume (tc, vertnbr >= 8);

  HEGEL_ASSERT (graphGenBuild (&finegrafdat, 0, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  coarmulttab = malloc (vertnbr * 2 * sizeof (SCOTCH_Num));

  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
  rc = SCOTCH_graphCoarsen (&finegrafdat, 1, 0.8, SCOTCH_COARSENNONE,
                            &coargrafdat, coarmulttab);
  HEGEL_ASSERT (rc != 2, "graphCoarsen error");
  hegel_assume (tc, rc == 0);

  SCOTCH_graphSize (&coargrafdat, &coarvertnbr, NULL);
  SCOTCH_graphData (&coargrafdat, &coarbaseval, NULL,
                    &coarverttab, NULL, NULL, NULL, NULL, &coaredgetab, NULL);

  /* Build fine-to-coarse mapping from coarmulttab */
  finemap = malloc (vertnbr * sizeof (SCOTCH_Num));
  memset (finemap, -1, vertnbr * sizeof (SCOTCH_Num));

  for (coarvertnum = 0; coarvertnum < coarvertnbr; coarvertnum ++) {
    SCOTCH_Num          fine0, fine1;

    fine0 = coarmulttab[2 * coarvertnum];
    fine1 = coarmulttab[2 * coarvertnum + 1];

    HEGEL_ASSERT (fine0 >= 0 && fine0 < vertnbr,
                  "coarmulttab[%d].0 = %d out of range",
                  (int) coarvertnum, (int) fine0);
    HEGEL_ASSERT (fine1 >= 0 && fine1 < vertnbr,
                  "coarmulttab[%d].1 = %d out of range",
                  (int) coarvertnum, (int) fine1);

    finemap[fine0] = coarvertnum;
    finemap[fine1] = coarvertnum;
  }

  /* Property: all fine vertices are mapped */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    HEGEL_ASSERT (finemap[vertnum] >= 0,
                  "fine vertex %d not mapped to any coarse vertex",
                  (int) vertnum);
  }

  /* Property: for each fine edge (u,v) with finemap[u] != finemap[v],
  ** edge (finemap[u], finemap[v]) must exist in coarse graph */
  for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
    SCOTCH_Num          edgenum;

    for (edgenum = verttab[vertnum]; edgenum < verttab[vertnum + 1]; edgenum ++) {
      SCOTCH_Num          vertend;
      SCOTCH_Num          cu, cv;

      vertend = edgetab[edgenum];
      cu = finemap[vertnum];
      cv = finemap[vertend];

      if (cu != cv) {
        /* Check that (cu, cv) exists in coarse graph */
        SCOTCH_Num          ce;
        int                 found;

        found = 0;
        for (ce = coarverttab[cu + coarbaseval] - coarbaseval;
             ce < coarverttab[cu + coarbaseval + 1] - coarbaseval; ce ++) {
          if (coaredgetab[ce] - coarbaseval == cv) {
            found = 1;
            break;
          }
        }
        HEGEL_ASSERT (found,
                      "fine edge (%d,%d) -> coarse (%d,%d) not in coarse graph",
                      (int) vertnum, (int) vertend, (int) cu, (int) cv);
      }
    }
  }

  free (finemap);
  SCOTCH_graphExit (&coargrafdat);
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

  printf ("Running coarsening matching consistency test...\n");
  hegel_run_test (testCoarsenMatching);
  printf ("PASSED\n");

  return (0);
}
