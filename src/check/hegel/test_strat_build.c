#include <stdio.h>
#include <stdlib.h>

#include "scotch.h"
#include "hegel_c.h"

/*
** Property: strategy builders always succeed for valid flag combinations.
*/
static
void
testStratBuild (
hegel_testcase *            tc)
{
  SCOTCH_Strat        stratdat;
  int                 buildertype;

  HEGEL_ASSERT (SCOTCH_stratInit (&stratdat) == 0, "stratInit failed");

  buildertype = hegel_draw_int (tc, 0, 2);

  switch (buildertype) {
    case 0: {                                     /* stratGraphMapBuild */
      SCOTCH_Num          flagval;
      SCOTCH_Num          partnbr;
      double              balrat;
      int                 flags[4] = { SCOTCH_STRATDEFAULT, SCOTCH_STRATRECURSIVE,
                                       SCOTCH_STRATREMAP,
                                       SCOTCH_STRATRECURSIVE | SCOTCH_STRATREMAP };
      int                 flagidx;

      flagidx = hegel_draw_int (tc, 0, 3);
      flagval = flags[flagidx];
      partnbr = hegel_draw_int (tc, 2, 64);
      balrat  = (double) hegel_draw_int (tc, 1, 100) / 1000.0; /* 0.001 to 0.1 */

      HEGEL_ASSERT (SCOTCH_stratGraphMapBuild (&stratdat, flagval, partnbr, balrat) == 0,
                    "stratGraphMapBuild(flag=%d, nparts=%d, bal=%f) failed",
                    (int) flagval, (int) partnbr, balrat);
      break;
    }
    case 1: {                                     /* stratGraphOrderBuild */
      SCOTCH_Num          flagval;
      SCOTCH_Num          levlnbr;
      double              balrat;
      int                 flags[8] = { SCOTCH_STRATDEFAULT,
                                       SCOTCH_STRATDISCONNECTED,
                                       SCOTCH_STRATLEVELMAX,
                                       SCOTCH_STRATLEVELMIN,
                                       SCOTCH_STRATLEVELMAX | SCOTCH_STRATLEVELMIN,
                                       SCOTCH_STRATLEAFSIMPLE,
                                       SCOTCH_STRATSEPASIMPLE,
                                       SCOTCH_STRATDISCONNECTED | SCOTCH_STRATLEVELMAX };
      int                 flagidx;

      flagidx = hegel_draw_int (tc, 0, 7);
      flagval = flags[flagidx];
      levlnbr = hegel_draw_int (tc, 0, 10);
      balrat  = (double) hegel_draw_int (tc, 1, 500) / 1000.0; /* 0.001 to 0.5 */

      HEGEL_ASSERT (SCOTCH_stratGraphOrderBuild (&stratdat, flagval, levlnbr, balrat) == 0,
                    "stratGraphOrderBuild(flag=%d, levl=%d, bal=%f) failed",
                    (int) flagval, (int) levlnbr, balrat);
      break;
    }
    case 2: {                                     /* stratGraphClusterBuild */
      SCOTCH_Num          flagval;
      SCOTCH_Num          partnbr;
      double              pwght;
      double              balrat;
      int                 flags[2] = { SCOTCH_STRATDEFAULT, SCOTCH_STRATQUALITY };
      int                 flagidx;

      flagidx = hegel_draw_int (tc, 0, 1);
      flagval = flags[flagidx];
      partnbr = hegel_draw_int (tc, 2, 32);
      pwght   = (double) hegel_draw_int (tc, 1, 100) / 100.0;  /* 0.01 to 1.0 */
      balrat  = (double) hegel_draw_int (tc, 1, 100) / 1000.0; /* 0.001 to 0.1 */

      HEGEL_ASSERT (SCOTCH_stratGraphClusterBuild (&stratdat, flagval, partnbr, pwght, balrat) == 0,
                    "stratGraphClusterBuild(flag=%d, nparts=%d, pw=%f, bal=%f) failed",
                    (int) flagval, (int) partnbr, pwght, balrat);
      break;
    }
  }

  SCOTCH_stratExit (&stratdat);
}

int
main (
int                 argc,
char *              argv[])
{
  (void) argc;
  (void) argv;

  printf ("Running strategy builder property test...\n");
  hegel_run_test (testStratBuild);
  printf ("PASSED\n");

  return (0);
}
