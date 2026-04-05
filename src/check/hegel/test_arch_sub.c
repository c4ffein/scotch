#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Property: SCOTCH_archSub creates a sub-architecture.
** Size equals selected node count; save/load roundtrip works.
** Uses a fixed subset to avoid variable draw counts.
*/
static
void
testArchSub (
hegel_testcase *            tc)
{
  SCOTCH_Arch         archdat;
  SCOTCH_Arch         subarchdat;
  SCOTCH_Arch         subarchdat2;
  SCOTCH_Num          archsiz;
  SCOTCH_Num          subnbr;
  SCOTCH_Num *        subtab;
  SCOTCH_Num          subarchsiz;
  SCOTCH_Num          subarchsiz2;
  SCOTCH_Num          i;
  FILE *              fileptr;

  HEGEL_ASSERT (SCOTCH_archInit (&archdat) == 0, "archInit failed");

  archsiz = hegel_draw_int (tc, 4, 16);
  HEGEL_ASSERT (SCOTCH_archCmplt (&archdat, archsiz) == 0,
                "archCmplt(%d) failed", (int) archsiz);

  /* Take the first subnbr nodes (deterministic, no rejection loop) */
  subnbr = hegel_draw_int (tc, 1, archsiz);
  subtab = malloc (subnbr * sizeof (SCOTCH_Num));
  for (i = 0; i < subnbr; i ++)
    subtab[i] = i;

  HEGEL_ASSERT (SCOTCH_archSub (&subarchdat, &archdat, subnbr, subtab) == 0,
                "archSub failed");

  subarchsiz = SCOTCH_archSize (&subarchdat);
  HEGEL_ASSERT (subarchsiz == subnbr,
                "sub archSize %d != subnbr %d", (int) subarchsiz, (int) subnbr);

  /* Save/load roundtrip */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_archSave (&subarchdat, fileptr) == 0, "archSave failed");

  rewind (fileptr);
  HEGEL_ASSERT (SCOTCH_archInit (&subarchdat2) == 0, "archInit(sub2) failed");
  HEGEL_ASSERT (SCOTCH_archLoad (&subarchdat2, fileptr) == 0, "archLoad failed");
  fclose (fileptr);

  subarchsiz2 = SCOTCH_archSize (&subarchdat2);
  HEGEL_ASSERT (subarchsiz2 == subarchsiz,
                "archSize mismatch: %d vs %d",
                (int) subarchsiz, (int) subarchsiz2);

  SCOTCH_archExit (&subarchdat2);
  SCOTCH_archExit (&subarchdat);
  free (subtab);
  SCOTCH_archExit (&archdat);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running archSub property test...\n");
  hegel_run_test (testArchSub);
  printf ("PASSED\n");

  return (0);
}
