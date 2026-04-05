#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Property: architecture save/load roundtrip preserves archName and archSize.
*/
static
void
testArchRoundtrip (
hegel_testcase *            tc)
{
  SCOTCH_Arch         archdat;
  SCOTCH_Arch         archdat2;
  SCOTCH_Num          archsiz;
  SCOTCH_Num          archsiz2;
  char *              archnam;
  char *              archnam2;
  FILE *              fileptr;
  int                 archtype;

  HEGEL_ASSERT (SCOTCH_archInit (&archdat) == 0, "archInit failed");

  /* Pick a random architecture type */
  archtype = hegel_draw_int (tc, 0, 4);

  switch (archtype) {
    case 0: {                                     /* Complete graph */
      SCOTCH_Num          nparts;

      nparts = hegel_draw_int (tc, 2, 32);
      HEGEL_ASSERT (SCOTCH_archCmplt (&archdat, nparts) == 0,
                    "archCmplt(%d) failed", (int) nparts);
      break;
    }
    case 1: {                                     /* 2D mesh */
      SCOTCH_Num          dim0, dim1;

      dim0 = hegel_draw_int (tc, 1, 8);
      dim1 = hegel_draw_int (tc, 1, 8);
      HEGEL_ASSERT (SCOTCH_archMesh2 (&archdat, dim0, dim1) == 0,
                    "archMesh2(%d,%d) failed", (int) dim0, (int) dim1);
      break;
    }
    case 2: {                                     /* 3D mesh */
      SCOTCH_Num          dim0, dim1, dim2;

      dim0 = hegel_draw_int (tc, 1, 5);
      dim1 = hegel_draw_int (tc, 1, 5);
      dim2 = hegel_draw_int (tc, 1, 5);
      HEGEL_ASSERT (SCOTCH_archMesh3 (&archdat, dim0, dim1, dim2) == 0,
                    "archMesh3(%d,%d,%d) failed", (int) dim0, (int) dim1, (int) dim2);
      break;
    }
    case 3: {                                     /* 2D torus */
      SCOTCH_Num          dim0, dim1;

      dim0 = hegel_draw_int (tc, 1, 8);
      dim1 = hegel_draw_int (tc, 1, 8);
      HEGEL_ASSERT (SCOTCH_archTorus2 (&archdat, dim0, dim1) == 0,
                    "archTorus2(%d,%d) failed", (int) dim0, (int) dim1);
      break;
    }
    case 4: {                                     /* Hypercube */
      SCOTCH_Num          dim;

      dim = hegel_draw_int (tc, 1, 6);
      HEGEL_ASSERT (SCOTCH_archHcub (&archdat, dim) == 0,
                    "archHcub(%d) failed", (int) dim);
      break;
    }
  }

  archsiz = SCOTCH_archSize (&archdat);
  archnam = SCOTCH_archName (&archdat);
  HEGEL_ASSERT (archnam != NULL, "archName returned NULL");
  HEGEL_ASSERT (archsiz > 0, "archSize returned %d", (int) archsiz);

  /* Save to temp file */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_archSave (&archdat, fileptr) == 0, "archSave failed");

  /* Rewind and load */
  rewind (fileptr);
  HEGEL_ASSERT (SCOTCH_archInit (&archdat2) == 0, "archInit(2) failed");
  HEGEL_ASSERT (SCOTCH_archLoad (&archdat2, fileptr) == 0, "archLoad failed");
  fclose (fileptr);

  /* Property: name matches */
  archnam2 = SCOTCH_archName (&archdat2);
  HEGEL_ASSERT (archnam2 != NULL, "archName(2) returned NULL");
  HEGEL_ASSERT (strcmp (archnam, archnam2) == 0,
                "archName mismatch: '%s' vs '%s'", archnam, archnam2);

  /* Property: size matches */
  archsiz2 = SCOTCH_archSize (&archdat2);
  HEGEL_ASSERT (archsiz2 == archsiz,
                "archSize mismatch: %d vs %d", (int) archsiz, (int) archsiz2);

  SCOTCH_archExit (&archdat2);
  SCOTCH_archExit (&archdat);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running architecture save/load roundtrip test...\n");
  hegel_run_test (testArchRoundtrip);
  printf ("PASSED\n");

  return (0);
}
