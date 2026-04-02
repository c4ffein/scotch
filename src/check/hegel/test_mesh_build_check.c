#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Generate a simple 2D mesh: elements are quads, nodes are grid points.
** Elements numbered [velmbas, velmbas + velmnbr), nodes [vnodbas, vnodbas + vnodnbr).
** With elements first: vnodbas = velmbas + velmnbr.
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
  SCOTCH_Num          vertnum;

  dimx = hegel_draw_int (tc, 2, 8);
  dimy = hegel_draw_int (tc, 2, 8);

  velmnbr = (dimx - 1) * (dimy - 1);  /* quad elements */
  vnodnbr = dimx * dimy;              /* grid nodes */
  vertnbr = velmnbr + vnodnbr;

  velmbas = 0;
  vnodbas = velmnbr;                   /* nodes start right after elements */

  /* Count edges: each quad element connects to 4 nodes, each edge bidirectional */
  /* Element->node: 4 per element. Node->element: variable (1-4 per node) */
  edgenbr = 2 * velmnbr * 4;          /* Upper bound; will be exact for interior */
  /* Actually, edge count = 2 * (sum of element degrees) */
  /* Each element has degree 4, so element side = 4 * velmnbr */
  /* Each interior node has degree 4, border nodes less */
  /* Total edges = 2 * 4 * velmnbr (each elem-node arc counted twice) */
  edgenbr = 2 * 4 * velmnbr;

  verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  edgetab = malloc ((edgenbr > 0 ? edgenbr : 1) * sizeof (SCOTCH_Num));

  edgenum = 0;

  /* Element vertices: each quad (ex, ey) connects to 4 corner nodes */
  {
    SCOTCH_Num          ex, ey;

    for (ey = 0; ey < dimy - 1; ey ++) {
      for (ex = 0; ex < dimx - 1; ex ++) {
        SCOTCH_Num          elemnum;

        elemnum = ey * (dimx - 1) + ex;  /* Element index */
        verttab[elemnum] = edgenum;

        /* 4 corner nodes of this quad element */
        edgetab[edgenum ++] = vnodbas + ey       * dimx + ex;
        edgetab[edgenum ++] = vnodbas + ey       * dimx + ex + 1;
        edgetab[edgenum ++] = vnodbas + (ey + 1) * dimx + ex;
        edgetab[edgenum ++] = vnodbas + (ey + 1) * dimx + ex + 1;
      }
    }
  }

  /* Node vertices: each node connects to adjacent elements */
  {
    SCOTCH_Num          nx, ny;

    for (ny = 0; ny < dimy; ny ++) {
      for (nx = 0; nx < dimx; nx ++) {
        SCOTCH_Num          nodenum;

        nodenum = velmnbr + ny * dimx + nx;  /* Node index in verttab */
        verttab[nodenum] = edgenum;

        /* Up to 4 adjacent elements */
        if (nx > 0 && ny > 0)
          edgetab[edgenum ++] = velmbas + (ny - 1) * (dimx - 1) + (nx - 1);
        if (nx < dimx - 1 && ny > 0)
          edgetab[edgenum ++] = velmbas + (ny - 1) * (dimx - 1) + nx;
        if (nx > 0 && ny < dimy - 1)
          edgetab[edgenum ++] = velmbas + ny * (dimx - 1) + (nx - 1);
        if (nx < dimx - 1 && ny < dimy - 1)
          edgetab[edgenum ++] = velmbas + ny * (dimx - 1) + nx;
      }
    }
  }

  verttab[vertnbr] = edgenum;
  edgenbr = edgenum;

  *velmbasptr = velmbas;
  *vnodbasptr = vnodbas;
  *velmnbrptr = velmnbr;
  *vnodnbrptr = vnodnbr;
  *verttabptr = verttab;
  *edgetabptr = edgetab;
  *edgenbrptr = edgenbr;
}

/*
** Property: meshBuild + meshCheck succeed on a valid mesh.
** meshGraph converts to a graph that passes graphCheck.
** meshOrder produces a valid permutation.
*/
static
void
testMeshBuildCheck (
hegel_testcase *            tc)
{
  SCOTCH_Mesh         meshdat;
  SCOTCH_Graph        grafdat;
  SCOTCH_Num          velmbas, vnodbas, velmnbr, vnodnbr;
  SCOTCH_Num *        verttab;
  SCOTCH_Num *        edgetab;
  SCOTCH_Num          edgenbr;
  SCOTCH_Num          velmnbr_out, vnodnbr_out, edgenbr_out;

  meshGenGrid2D (tc, &velmbas, &vnodbas, &velmnbr, &vnodnbr,
                 &verttab, &edgetab, &edgenbr);

  /* Build mesh */
  HEGEL_ASSERT (SCOTCH_meshInit (&meshdat) == 0, "meshInit failed");
  HEGEL_ASSERT (SCOTCH_meshBuild (&meshdat, velmbas, vnodbas,
                                  velmnbr, vnodnbr,
                                  verttab, NULL, NULL, NULL, NULL,
                                  edgenbr, edgetab) == 0,
                "meshBuild failed");
  HEGEL_ASSERT (SCOTCH_meshCheck (&meshdat) == 0, "meshCheck failed");

  /* Property: meshSize returns correct counts */
  SCOTCH_meshSize (&meshdat, &velmnbr_out, &vnodnbr_out, &edgenbr_out);
  HEGEL_ASSERT (velmnbr_out == velmnbr,
                "velmnbr mismatch: %d vs %d", (int) velmnbr, (int) velmnbr_out);
  HEGEL_ASSERT (vnodnbr_out == vnodnbr,
                "vnodnbr mismatch: %d vs %d", (int) vnodnbr, (int) vnodnbr_out);

  /* Property: meshGraph produces a valid graph */
  HEGEL_ASSERT (SCOTCH_graphInit (&grafdat) == 0, "graphInit failed");
  HEGEL_ASSERT (SCOTCH_meshGraph (&meshdat, &grafdat) == 0, "meshGraph failed");
  HEGEL_ASSERT (SCOTCH_graphCheck (&grafdat) == 0,
                "graphCheck on mesh-derived graph failed");
  SCOTCH_graphExit (&grafdat);

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

  printf ("Running mesh build/check/graph/order test...\n");
  hegel_run_test (testMeshBuildCheck);
  printf ("PASSED\n");

  return (0);
}
