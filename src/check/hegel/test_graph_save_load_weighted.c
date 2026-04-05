#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: saving and loading a weighted graph (with velotab and edlotab)
** preserves vertex count, edge count, vertex weights, and edge weights.
*/
static
void
testSaveLoadWeighted (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Graph        grafdat2;
  SCOTCH_Num          baseval;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num *        velotab;
  SCOTCH_Num *        edlotab;
  SCOTCH_Num          vertnum;
  FILE *              fileptr;

  graphGenAny (tc, &baseval, &vertnbr, &verttab, &edgetab, &edgenbr);

  /* Need edges for edge weights */
  hegel_assume (tc, edgenbr > 0);

  /* Generate vertex weights */
  velotab = malloc (vertnbr * sizeof (SCOTCH_Num));
  for (vertnum = 0; vertnum < vertnbr; vertnum ++)
    velotab[vertnum] = hegel_draw_int (tc, 1, 100);

  /* Generate symmetric edge weights */
  edlotab = calloc (edgenbr, sizeof (SCOTCH_Num));
  {
    SCOTCH_Num          v;

    for (v = 0; v < vertnbr; v ++) {
      SCOTCH_Num          start, end, e;

      start = verttab[v] - baseval;
      end   = verttab[v + 1] - baseval;
      for (e = start; e < end; e ++) {
        if (edlotab[e] == 0) {                      /* Not yet assigned */
          SCOTCH_Num          neighbor;
          SCOTCH_Num          nstart, nend, ne;
          SCOTCH_Num          weight;

          weight     = hegel_draw_int (tc, 1, 100);
          edlotab[e] = weight;
          neighbor   = edgetab[e] - baseval;

          /* Find reverse arc and set same weight */
          nstart = verttab[neighbor] - baseval;
          nend   = verttab[neighbor + 1] - baseval;
          for (ne = nstart; ne < nend; ne ++) {
            if (edgetab[ne] - baseval == v) {
              edlotab[ne] = weight;
              break;
            }
          }
        }
      }
    }
  }

  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_graphBuild (&grafdat, baseval, vertnbr,
                                   verttab, NULL, velotab, NULL,
                                   edgenbr, edgetab, edlotab) == 0,
                "graphBuild failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                "graphCheck failed on weighted graph");

  /* Save to temp file */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_graphSave (&grafdat, fileptr) == 0, "graphSave failed");

  /* Load back */
  rewind (fileptr);
  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat2) == 0, "graphInit(2) failed");
  HEGEL_ASSERT (SCOTCH_graphLoad (&grafdat2, fileptr, -1, 0) == 0, "graphLoad failed");
  fclose (fileptr);

  /* Property: loaded graph passes graphCheck */
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat2) == 0, "graphCheck on loaded graph failed");

  /* Property: vertex and edge counts match */
  {
    SCOTCH_Num          baseval2;
    SCOTCH_Num          vertnbr2;
    SCOTCH_Num          edgenbr2;
    SCOTCH_Num *        verttab2;
    SCOTCH_Num *        velotab2;
    SCOTCH_Num *        edgetab2;
    SCOTCH_Num *        edlotab2;

    SCOTCH_graphData (&grafdat2, &baseval2, &vertnbr2,
                      &verttab2, NULL, &velotab2, NULL,
                      &edgenbr2, &edgetab2, &edlotab2);

    HEGEL_ASSERT (vertnbr2 == vertnbr,
                  "vertnbr mismatch: %d vs %d", (int) vertnbr, (int) vertnbr2);
    HEGEL_ASSERT (edgenbr2 == edgenbr,
                  "edgenbr mismatch: %d vs %d", (int) edgenbr, (int) edgenbr2);

    /* Property: vertex weights preserved */
    HEGEL_ASSERT (velotab2 != NULL, "velotab lost after load");
    for (vertnum = 0; vertnum < vertnbr; vertnum ++) {
      HEGEL_ASSERT (velotab2[vertnum] == velotab[vertnum],
                    "velotab[%d]: %d vs %d",
                    (int) vertnum, (int) velotab[vertnum], (int) velotab2[vertnum]);
    }

    /* Property: edge weights preserved (accounting for possible rebase) */
    HEGEL_ASSERT (edlotab2 != NULL, "edlotab lost after load");
    {
      SCOTCH_Num          v;

      for (v = 0; v < vertnbr; v ++) {
        SCOTCH_Num          deg1, deg2;
        SCOTCH_Num          start1, start2;
        SCOTCH_Num          i;

        deg1   = verttab[v + 1] - verttab[v];
        deg2   = verttab2[v + 1] - verttab2[v];
        HEGEL_ASSERT (deg1 == deg2,
                      "degree mismatch at vertex %d: %d vs %d",
                      (int) v, (int) deg1, (int) deg2);

        start1 = verttab[v] - baseval;
        start2 = verttab2[v] - baseval2;
        for (i = 0; i < deg1; i ++) {
          HEGEL_ASSERT (edlotab2[start2 + i] == edlotab[start1 + i],
                        "edlotab mismatch at vertex %d edge %d: %d vs %d",
                        (int) v, (int) i,
                        (int) edlotab[start1 + i], (int) edlotab2[start2 + i]);
        }
      }
    }
  }

  SCOTCH_graphExit (&grafdat2);
  SCOTCH_graphExit (&grafdat);
  free (edlotab);
  free (velotab);
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

  printf ("Running weighted graph save/load roundtrip test...\n");
  hegel_run_test (testSaveLoadWeighted);
  printf ("PASSED\n");

  return (0);
}
