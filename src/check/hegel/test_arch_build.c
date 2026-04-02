#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"
#include "graph_gen.h"

/*
** Property: SCOTCH_archBuild2 constructs a decomposition-defined
** architecture from a graph. Pass NULL for listtab to use all vertices.
** The resulting architecture has vertnbr terminal nodes.
*/
static
void
testArchBuild (
hegel_testcase *            tc)
{
  SCOTCH_Graph        grafdat;
  SCOTCH_Arch         archdat;
  SCOTCH_Arch         archdat2;
  SCOTCH_Num          vertnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          archsiz;
  SCOTCH_Num          archsiz2;
  char *              archnam;
  char *              archnam2;
  FILE *              fileptr;

  /* archBuild2 requires baseval == 0 and well-connected graphs */
  graphGenGrid2D (tc, 0, &vertnbr, &verttab, &edgetab, &edgenbr);

  HEGEL_ASSERT (graphGenBuild (&grafdat, 0, vertnbr, verttab, edgetab, edgenbr) == 0,
                "graphGenBuild failed");

  /* Build architecture from entire graph (NULL = all vertices) */
  HEGEL_ASSERT (SCOTCH_archInit (&archdat) == 0, "archInit failed");
  HEGEL_ASSERT (SCOTCH_archBuild2 (&archdat, &grafdat, vertnbr, NULL) == 0,
                "archBuild2 failed");

  archsiz = SCOTCH_archSize (&archdat);
  HEGEL_ASSERT (archsiz == vertnbr,
                "archSize %d != vertnbr %d", (int) archsiz, (int) vertnbr);

  archnam = SCOTCH_archName (&archdat);
  HEGEL_ASSERT (archnam != NULL, "archName returned NULL");

  /* Save/load roundtrip */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_archSave (&archdat, fileptr) == 0, "archSave failed");

  rewind (fileptr);
  HEGEL_ASSERT (SCOTCH_archInit (&archdat2) == 0, "archInit(2) failed");
  HEGEL_ASSERT (SCOTCH_archLoad (&archdat2, fileptr) == 0, "archLoad failed");
  fclose (fileptr);

  archnam2 = SCOTCH_archName (&archdat2);
  archsiz2 = SCOTCH_archSize (&archdat2);
  HEGEL_ASSERT (strcmp (archnam, archnam2) == 0,
                "archName mismatch: '%s' vs '%s'", archnam, archnam2);
  HEGEL_ASSERT (archsiz2 == archsiz,
                "archSize mismatch: %d vs %d", (int) archsiz, (int) archsiz2);

  SCOTCH_archExit (&archdat2);
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

  printf ("Running archBuild property test...\n");
  hegel_run_test (testArchBuild);
  printf ("PASSED\n");

  return (0);
}
