#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Property: architecture domain operations are consistent.
** archDomFrst gives root domain, archDomBipart splits it,
** and domain sizes/weights are sane.
*/
static
void
testArchDom (
hegel_testcase *            tc)
{
  SCOTCH_Arch         archdat;
  SCOTCH_ArchDom      domdat;
  SCOTCH_ArchDom      dom0dat;
  SCOTCH_ArchDom      dom1dat;
  SCOTCH_Num          archsiz;
  SCOTCH_Num          domsiz;
  SCOTCH_Num          dom0siz;
  SCOTCH_Num          dom1siz;
  SCOTCH_Num          domwgt;
  SCOTCH_Num          nparts;
  int                 archtype;
  int                 rc;

  HEGEL_ASSERT (SCOTCH_archInit (&archdat) == 0, "archInit failed");

  archtype = hegel_draw_int (tc, 0, 2);
  switch (archtype) {
    case 0:
      nparts = hegel_draw_int (tc, 2, 32);
      HEGEL_ASSERT (SCOTCH_archCmplt (&archdat, nparts) == 0, "archCmplt failed");
      break;
    case 1: {
      SCOTCH_Num d0, d1;
      d0 = hegel_draw_int (tc, 2, 6);
      d1 = hegel_draw_int (tc, 2, 6);
      HEGEL_ASSERT (SCOTCH_archMesh2 (&archdat, d0, d1) == 0, "archMesh2 failed");
      break;
    }
    case 2: {
      SCOTCH_Num dim;
      dim = hegel_draw_int (tc, 1, 5);
      HEGEL_ASSERT (SCOTCH_archHcub (&archdat, dim) == 0, "archHcub failed");
      break;
    }
  }

  archsiz = SCOTCH_archSize (&archdat);

  /* Get first (root) domain */
  HEGEL_ASSERT (SCOTCH_archDomFrst (&archdat, &domdat) == 0, "archDomFrst failed");

  /* Property: root domain size == architecture size */
  domsiz = SCOTCH_archDomSize (&archdat, &domdat);
  HEGEL_ASSERT (domsiz == archsiz,
                "root domain size %d != archSize %d",
                (int) domsiz, (int) archsiz);

  /* Property: root domain weight > 0 */
  domwgt = SCOTCH_archDomWght (&archdat, &domdat);
  HEGEL_ASSERT (domwgt > 0, "root domain weight %d <= 0", (int) domwgt);

  /* Property: bipartition of root splits into two sub-domains whose sizes sum to root size */
  if (archsiz >= 2) {
    rc = SCOTCH_archDomBipart (&archdat, &domdat, &dom0dat, &dom1dat);
    HEGEL_ASSERT (rc == 0, "archDomBipart failed on root domain");

    dom0siz = SCOTCH_archDomSize (&archdat, &dom0dat);
    dom1siz = SCOTCH_archDomSize (&archdat, &dom1dat);

    HEGEL_ASSERT (dom0siz > 0 && dom1siz > 0,
                  "bipart produced empty domain: %d, %d",
                  (int) dom0siz, (int) dom1siz);
    HEGEL_ASSERT (dom0siz + dom1siz == domsiz,
                  "bipart sizes %d + %d != %d",
                  (int) dom0siz, (int) dom1siz, (int) domsiz);

    /* Property: distance between sub-domains is non-negative */
    {
      SCOTCH_Num          dist;

      dist = SCOTCH_archDomDist (&archdat, &dom0dat, &dom1dat);
      HEGEL_ASSERT (dist >= 0, "negative domain distance: %d", (int) dist);
    }

    /* Property: terminal domains can be enumerated */
    {
      SCOTCH_ArchDom      termdom;
      SCOTCH_Num          termnum;

      termnum = SCOTCH_archDomNum (&archdat, &dom0dat);
      /* For leaf domains, archDomTerm should succeed */
      if (dom0siz == 1) {
        HEGEL_ASSERT (SCOTCH_archDomTerm (&archdat, &termdom, termnum) == 0,
                      "archDomTerm failed for terminal %d", (int) termnum);
      }
    }
  }

  SCOTCH_archExit (&archdat);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running architecture domain operations test...\n");
  hegel_run_test (testArchDom);
  printf ("PASSED\n");

  return (0);
}
