#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Reuse the grid mesh generator (same as test_mesh_build_check).
*/
static
void
meshGenGrid2D (
hegel_testcase *            tc,
SCOTCH_Num *                velmbasptr,
SCOTCH_Num *                vnodbasptr,
SCOTCH_Num *                velmnbrptr,
SCOTCH_Num *                vnodnbrptr,
SCOTCH_Num **               verttabptr,
SCOTCH_Num **               edgetabptr,
SCOTCH_Num *                edgenbrptr)
{
  SCOTCH_Num          dimx, dimy;
  SCOTCH_Num          velmnbr, vnodnbr, vertnbr;
  SCOTCH_Num          velmbas, vnodbas;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          edgenum;

  dimx = hegel_draw_int (tc, 2, 8);
  dimy = hegel_draw_int (tc, 2, 8);
  velmnbr = (dimx - 1) * (dimy - 1);
  vnodnbr = dimx * dimy;
  vertnbr = velmnbr + vnodnbr;
  velmbas = 0;
  vnodbas = velmnbr;
  edgenbr = 2 * 4 * velmnbr;

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));
  edgenum = 0;

  {
    SCOTCH_Num ex, ey;
    for (ey = 0; ey < dimy - 1; ey ++)
      for (ex = 0; ex < dimx - 1; ex ++) {
        verttab[ey * (dimx - 1) + ex] = edgenum;
        edgetab[edgenum ++] = vnodbas + ey * dimx + ex;
        edgetab[edgenum ++] = vnodbas + ey * dimx + ex + 1;
        edgetab[edgenum ++] = vnodbas + (ey + 1) * dimx + ex;
        edgetab[edgenum ++] = vnodbas + (ey + 1) * dimx + ex + 1;
      }
  }
  {
    SCOTCH_Num nx, ny;
    for (ny = 0; ny < dimy; ny ++)
      for (nx = 0; nx < dimx; nx ++) {
        verttab[velmnbr + ny * dimx + nx] = edgenum;
        if (nx > 0 && ny > 0)         edgetab[edgenum ++] = velmbas + (ny - 1) * (dimx - 1) + (nx - 1);
        if (nx < dimx-1 && ny > 0)    edgetab[edgenum ++] = velmbas + (ny - 1) * (dimx - 1) + nx;
        if (nx > 0 && ny < dimy-1)    edgetab[edgenum ++] = velmbas + ny * (dimx - 1) + (nx - 1);
        if (nx < dimx-1 && ny < dimy-1) edgetab[edgenum ++] = velmbas + ny * (dimx - 1) + nx;
      }
  }
  verttab[vertnbr] = edgenum;
  edgenbr = edgenum;

  *velmbasptr = velmbas; *vnodbasptr = vnodbas;
  *velmnbrptr = velmnbr; *vnodnbrptr = vnodnbr;
  *verttabptr = verttab; *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Property: mesh save/load roundtrip preserves structure.
*/
static
void
testMeshSaveLoad (
hegel_testcase *            tc)
{
  SCOTCH_Mesh         meshdat;
  SCOTCH_Mesh         meshdat2;
  SCOTCH_Num          velmbas, vnodbas, velmnbr, vnodnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          velmnbr2, vnodnbr2;
  FILE *              fileptr;

  meshGenGrid2D (tc, &velmbas, &vnodbas, &velmnbr, &vnodnbr,
                 &verttab, &edgetab, &edgenbr);

  HEGEL_ASSERT (SCOTCH_meshInit (&meshdat) == 0, "meshInit failed");
  HEGEL_ASSERT (SCOTCH_meshBuild (&meshdat, velmbas, vnodbas,
                                  velmnbr, vnodnbr,
                                  verttab, NULL, NULL, NULL, NULL,
                                  edgenbr, edgetab) == 0,
                "meshBuild failed");
  HEGEL_ASSERT (SCOTCH_meshCheck (&meshdat) == 0, "meshCheck failed");

  /* Save */
  fileptr = tmpfile ();
  HEGEL_ASSERT (fileptr != NULL, "tmpfile failed");
  HEGEL_ASSERT (SCOTCH_meshSave (&meshdat, fileptr) == 0, "meshSave failed");

  /* Load */
  rewind (fileptr);
  HEGEL_ASSERT (SCOTCH_meshInit (&meshdat2) == 0, "meshInit(2) failed");
  HEGEL_ASSERT (SCOTCH_meshLoad (&meshdat2, fileptr, -1) == 0, "meshLoad failed");
  fclose (fileptr);

  /* Property: loaded mesh passes meshCheck */
  HEGEL_ASSERT (SCOTCH_meshCheck (&meshdat2) == 0,
                "meshCheck on loaded mesh failed");

  /* Property: element and node counts match */
  SCOTCH_meshSize (&meshdat2, &velmnbr2, &vnodnbr2, NULL);
  HEGEL_ASSERT (velmnbr2 == velmnbr,
                "velmnbr mismatch: %d vs %d", (int) velmnbr, (int) velmnbr2);
  HEGEL_ASSERT (vnodnbr2 == vnodnbr,
                "vnodnbr mismatch: %d vs %d", (int) vnodnbr, (int) vnodnbr2);

  SCOTCH_meshExit (&meshdat2);
  SCOTCH_meshExit (&meshdat);
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

  printf ("Running mesh save/load roundtrip test...\n");
  hegel_run_test (testMeshSaveLoad);
  printf ("PASSED\n");

  return (0);
}
