#ifndef SCOTCH_HELPERS_H
#define SCOTCH_HELPERS_H

#include "scotch.h"
#include "hegel_c.h"

/*
** Reset Scotch's global random state for deterministic behavior.
** Call at the start of every test function.
*/
static
void
scotchReset (void)
{
  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
}

/*
** Build a random valid mapping strategy string.
** The strategy uses recursive bipartitioning with a randomly
** selected separator method chain.
**
** Grammar for mapping: r{sep=<BIPART_CHAIN>,asc=<BIPART_CHAIN>}
** Bipartition methods: f{bal=X,move=N} h{pass=N} g{pass=N} m{...} z
*/
static
void
stratGenMap (
hegel_testcase *            tc,
char *                      buf,
int                         bufsiz)
{
  int                 sepmethod;
  int                 ascmethod;

  sepmethod = hegel_draw_int (tc, 0, 4);
  ascmethod = hegel_draw_int (tc, 0, 2);

  /* Build separator sub-strategy */
  switch (sepmethod) {
    case 0:
      snprintf (buf, bufsiz, "r{sep=f{bal=0.05,move=200}}");
      break;
    case 1:
      snprintf (buf, bufsiz, "r{sep=h{pass=10}}");
      break;
    case 2:
      snprintf (buf, bufsiz, "r{sep=m{vert=100,low=h{pass=10},asc=f{bal=0.05}}}");
      break;
    case 3:
      snprintf (buf, bufsiz,
        "r{sep=m{vert=80,low=h{pass=10},asc=b{width=3,bnd=f{bal=0.05},org=h{pass=10}}}}");
      break;
    default:
      snprintf (buf, bufsiz, "r{sep=g{pass=5}}");
      break;
  }

  (void) ascmethod;
}

/*
** Build a random valid ordering strategy string.
** Ordering methods: n{sep=...,ole=...,ose=...} e{...} s
*/
static
void
stratGenOrder (
hegel_testcase *            tc,
char *                      buf,
int                         bufsiz)
{
  int                 ordmethod;

  ordmethod = hegel_draw_int (tc, 0, 4);

  switch (ordmethod) {
    case 0: /* Nested dissection with FM separator */
      snprintf (buf, bufsiz,
        "n{sep=m{vert=100,low=h{pass=10},asc=b{width=3,bnd=f{bal=0.05},org=h{pass=10}}},ole=s,ose=s}");
      break;
    case 1: /* Nested dissection with greedy separator */
      snprintf (buf, bufsiz, "n{sep=h{pass=10},ole=s,ose=s}");
      break;
    case 2: /* Nested dissection with multi-level + diffusion */
      snprintf (buf, bufsiz,
        "n{sep=m{vert=80,low=h{pass=10},asc=f{bal=0.05}},ole=s,ose=s}");
      break;
    case 3: /* Approx minimum degree */
      snprintf (buf, bufsiz, "d{cmin=15,cmax=100000,frat=0.08}");
      break;
    case 4: /* Simple (natural) ordering */
      snprintf (buf, bufsiz, "s");
      break;
  }
}

/*
** Build a random valid bipartitioning strategy string.
** Used for the sep= parameter in mapping and ordering.
*/
static
void
stratGenBipart (
hegel_testcase *            tc,
char *                      buf,
int                         bufsiz)
{
  int                 bimethod;

  bimethod = hegel_draw_int (tc, 0, 5);

  switch (bimethod) {
    case 0: /* FM refinement */
      snprintf (buf, bufsiz, "f{bal=0.05,move=200}");
      break;
    case 1: /* Greedy growing */
      snprintf (buf, bufsiz, "h{pass=10}");
      break;
    case 2: /* Greedy partitioning */
      snprintf (buf, bufsiz, "g{pass=5}");
      break;
    case 3: /* Multi-level with FM */
      snprintf (buf, bufsiz, "m{vert=100,low=h{pass=10},asc=f{bal=0.05}}");
      break;
    case 4: /* Band + FM */
      snprintf (buf, bufsiz, "b{width=3,bnd=f{bal=0.05},org=h{pass=10}}");
      break;
    default: /* Zero (all to part 0) */
      snprintf (buf, bufsiz, "z");
      break;
  }
}

#endif /* SCOTCH_HELPERS_H */
