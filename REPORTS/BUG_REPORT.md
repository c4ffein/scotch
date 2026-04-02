# Bug report: `hgraphOrderCp` writes permutation at wrong offset when `ordenum != 0`

## Summary

`SCOTCH_graphOrder` returns 0 (success) but produces an invalid permutation — containing out-of-range values and missing entries — when the ordering strategy includes the `SCOTCH_STRATDISCONNECTED` flag and the graph has multiple connected components where at least one non-first component undergoes vertex compression.

The root cause is a single incorrect initializer in `hgraph_order_cp.c`: the compression expansion loop writes to `peritab[0..]` instead of `peritab[ordenum..]`, clobbering earlier components' results and leaving its own positions uninitialized.

## Affected versions

Confirmed in Scotch v7.0.11 ("Sankara"). The bug has been present since at least v6.0 (the `hgraphOrderCp` expansion code has not changed structurally since the function was introduced).

## Root cause

In `src/libscotch/hgraph_order_cp.c`, line 478, when the compressed ordering is expanded back to fine vertices, the position counter `finevsizsum` is initialized to `0` instead of `ordenum`:

```c
/* Line 478 — BEFORE (buggy) */
for (coarvertnum = coargrafdat.s.baseval, finevsizsum = 0;
     coarvertnum < coargrafdat.vnohnnd; coarvertnum ++) {
  coarvpostax[coarperitax[coarvertnum]] = finevsizsum;
  finevsizsum += coarvsiztax[coarperitax[coarvertnum]];
}
```

This causes the expansion to write fine vertex numbers into `peritab[0], peritab[1], ...` regardless of the value of `ordenum`. When `hgraphOrderCp` is called with `ordenum > 0` (as happens for non-first connected components under `SCOTCH_STRATDISCONNECTED`), the writes land at the wrong positions:

- Positions `peritab[0..k]` are overwritten, destroying earlier components' valid entries.
- Positions `peritab[ordenum..ordenum+k]` are never written, leaving them uninitialized.

Every other ordering method correctly accounts for `ordenum`:

| Method | How it uses `ordenum` |
|---|---|
| `hgraphOrderSi` | `peritab[ordenum + i] = ...` |
| `hgraphOrderHf` | passes `ordeptr->peritab + ordenum` to `hallOrderHxBuild` |
| `hgraphOrderHd` | passes `ordeptr->peritab + ordenum` to `hallOrderHxBuild` |
| `hgraphOrderNd` | passes adjusted `ordenum` to recursive sub-calls |
| `hgraphOrderCc` | passes `ordenum + roottab[rootnum]` to each component |
| `hgraphOrderCp` (non-compress path) | passes `ordenum` through to sub-strategy |
| **`hgraphOrderCp` (compress path)** | **ignores `ordenum` — BUG** |

## Trigger conditions

The bug requires all three conditions:

1. **`SCOTCH_STRATDISCONNECTED` flag** — this wraps the strategy in `o{strat=...}`, causing `hgraphOrderCc` to split the graph into connected components and order each with a different `ordenum`.

2. **Multiple connected components** — so that at least one component is ordered with `ordenum > 0`. This can be caused by isolated vertices, or simply by a disconnected graph. Isolated vertices are not required.

3. **Compression fires on a non-first component** — the component must have vertices with identical closed neighborhoods (same degree, same neighbors). The simplest case is two vertices connected only to each other (a K₂ subgraph). The compression ratio threshold (default 0.7) must also be met.

### Why isolated vertices make it easy to trigger

Isolated vertices are single-vertex components. They guarantee condition (2) and push all multi-vertex components to `ordenum > 0`. A K₂ pair (two vertices connected only to each other) always compresses because both vertices have the same closed neighborhood `{a, b}`. Random sparse graphs with 4–20 vertices very often contain both isolated vertices and K₂ pairs, which is why property-based testing found the bug quickly (~25% of random graphs trigger it).

### The bug is not limited to small graphs or isolated vertices

Any disconnected graph can trigger the bug. See the second reproducer below: a 502-vertex graph with no isolated vertices.

## Impact

- **Silent data corruption.** `SCOTCH_graphOrder` returns 0 (success) but `permtab` contains out-of-range values and/or missing vertex entries.
- **No detection in release builds.** The `orderCheck` function (called only in debug builds, `SCOTCH_DEBUG_ORDER2`) does detect the inconsistency and returns an error, but release builds skip the check entirely.
- **Affects downstream consumers.** Any code that uses `SCOTCH_STRATDISCONNECTED` for ordering (e.g., sparse direct solvers using Scotch for fill-reducing ordering) will silently receive an invalid permutation when the graph happens to be disconnected with compressible components.

## Fix

One-character change on line 478 of `src/libscotch/hgraph_order_cp.c`:

```diff
-  for (coarvertnum = coargrafdat.s.baseval, finevsizsum = 0; /* Compute initial indices for inverse permutation expansion */
+  for (coarvertnum = coargrafdat.s.baseval, finevsizsum = ordenum; /* Compute initial indices for inverse permutation expansion */
```

## Reproducer 1 — minimal (14 vertices, isolated vertices)

```c
#include <stdio.h>
#include <string.h>
#include "scotch.h"

int main () {
  SCOTCH_Graph g;
  SCOTCH_Strat s;

  SCOTCH_randomSeed (42);
  SCOTCH_randomReset ();
  SCOTCH_graphInit (&g);

  /* 14 vertices, 8 arcs (4 undirected edges).
  ** Components: {0,3,6,13}, {10,12}, and 8 isolated vertices.
  ** The pair {10,12} has identical adjacency and compresses. */
  SCOTCH_Num verttab[] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 8};
  SCOTCH_Num edgetab[] = {13, 13, 13, 12, 10, 0, 3, 6};
  SCOTCH_graphBuild (&g, 0, 14, verttab, NULL, NULL, NULL, 8, edgetab, NULL);
  SCOTCH_graphCheck (&g);

  SCOTCH_Num permtab[14];
  memset (permtab, 0xBB, sizeof (permtab));
  SCOTCH_stratInit (&s);
  SCOTCH_stratGraphOrderBuild (&s, SCOTCH_STRATDISCONNECTED, 3, 0.2);

  SCOTCH_Num cblknbr = 0;
  int rc = SCOTCH_graphOrder (&g, &s, permtab, NULL, &cblknbr, NULL, NULL);
  printf ("rc = %d\n", rc);

  int ok = 1;
  char seen[14] = {0};
  for (int i = 0; i < 14; i ++) {
    if (permtab[i] < 0 || permtab[i] >= 14) {
      printf ("permtab[%d] = %d — OUT OF RANGE\n", i, (int) permtab[i]);
      ok = 0;
    } else if (seen[permtab[i]]) {
      printf ("permtab[%d] = %d — DUPLICATE\n", i, (int) permtab[i]);
      ok = 0;
    } else {
      seen[permtab[i]] = 1;
    }
  }
  printf (ok ? "PASS\n" : "FAIL\n");

  SCOTCH_stratExit (&s);
  SCOTCH_graphExit (&g);
  return (ok ? 0 : 1);
}
```

**Output without fix:**
```
rc = 0
permtab[13] = -1145324613 — OUT OF RANGE
FAIL
```

## Reproducer 2 — large graph, no isolated vertices (502 vertices)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scotch.h"

/* Component 1: path 0 -- 1 -- ... -- 499  (500 vertices, 499 edges)
** Component 2: edge 500 -- 501             (2 vertices, 1 edge)
** No isolated vertices. The pair {500,501} compresses. */
int main () {
  SCOTCH_Graph g;
  SCOTCH_Strat s;
  int vertnbr = 502;
  int edgenbr = 1000;    /* 499*2 + 1*2 */
  SCOTCH_Num * verttab = malloc ((vertnbr + 1) * sizeof (SCOTCH_Num));
  SCOTCH_Num * edgetab = malloc (edgenbr * sizeof (SCOTCH_Num));
  SCOTCH_Num * permtab = malloc (vertnbr * sizeof (SCOTCH_Num));
  int pos = 0, i;

  for (i = 0; i < 500; i ++) {
    verttab[i] = pos;
    if (i > 0)   edgetab[pos ++] = i - 1;
    if (i < 499) edgetab[pos ++] = i + 1;
  }
  verttab[500] = pos;  edgetab[pos ++] = 501;
  verttab[501] = pos;  edgetab[pos ++] = 500;
  verttab[502] = pos;

  SCOTCH_graphInit (&g);
  SCOTCH_graphBuild (&g, 0, vertnbr, verttab, NULL,
                     NULL, NULL, edgenbr, edgetab, NULL);
  SCOTCH_graphCheck (&g);

  memset (permtab, 0xBB, vertnbr * sizeof (SCOTCH_Num));
  SCOTCH_stratInit (&s);
  SCOTCH_stratGraphOrderBuild (&s, SCOTCH_STRATDISCONNECTED, 3, 0.2);

  SCOTCH_Num cblknbr = 0;
  int rc = SCOTCH_graphOrder (&g, &s, permtab, NULL, &cblknbr, NULL, NULL);
  printf ("rc = %d\n", rc);

  int ok = 1;
  char * seen = calloc (vertnbr, 1);
  for (i = 0; i < vertnbr; i ++) {
    if (permtab[i] < 0 || permtab[i] >= vertnbr) {
      printf ("permtab[%d] = %d — OUT OF RANGE\n", i, (int) permtab[i]);
      ok = 0;
    } else if (seen[permtab[i]]) {
      printf ("permtab[%d] = %d — DUPLICATE\n", i, (int) permtab[i]);
      ok = 0;
    } else {
      seen[permtab[i]] = 1;
    }
  }
  printf (ok ? "PASS\n" : "FAIL\n");

  free (seen); free (permtab); free (edgetab); free (verttab);
  SCOTCH_stratExit (&s);
  SCOTCH_graphExit (&g);
  return (ok ? 0 : 1);
}
```

**Output without fix:**
```
rc = 0
permtab[485] = -1145324613 — OUT OF RANGE
permtab[486] = -1145324613 — OUT OF RANGE
FAIL
```

## How the bug was found

The bug was discovered through property-based testing (PBT) using the Hegel framework. A randomized test generates graphs of various sizes and topologies, applies `SCOTCH_stratGraphOrderBuild` with each flag variant, calls `SCOTCH_graphOrder`, and checks that the resulting `permtab` is a valid permutation (all values in range, no duplicates). The `SCOTCH_STRATDISCONNECTED` flag variant failed on the first test iteration.

## Detailed execution trace (reproducer 1)

For readers who want to understand the full mechanics:

1. `SCOTCH_stratGraphOrderBuild(&s, SCOTCH_STRATDISCONNECTED, 3, 0.2)` produces strategy string `o{strat=c{rat=0.7,cpr=n{...},unc=n{...}}}`.

2. `hgraphOrderCc` (the `o` method) does BFS to find 10 connected components and orders each separately:
   - Component 0: {0, 3, 6, 13} — 4 vertices, `ordenum = 0`
   - Components 1–7: isolated vertices {1}, {2}, {4}, {5}, {7}, {8}, {9}
   - Component 8: {10, 12} — 2 vertices, `ordenum = 11`
   - Component 9: {11} — isolated, `ordenum = 13`

3. Component 0 is ordered first. The chain `c → n → f → si` produces identity ordering. Writes `peritab[0..3] = [0, 13, 3, 6]`. Correct.

4. Isolated vertex components are each 1 vertex. Compression ratio check fails (1 > 1×0.7), so `hgraphOrderCp` delegates to the `unc` sub-strategy, which correctly passes `ordenum` through. Writes `peritab[4..10] = [1, 2, 4, 5, 7, 8, 9]`. Correct.

5. **Component 8 ({10, 12}) at `ordenum = 11`:** Both vertices have degree 1 and identical closed neighborhood {10, 12}. Hash values match (10+12 = 12+10 = 22). `hgraphOrderCp` compresses them into a single coarse vertex. The compressed graph is ordered (trivially). Then the expansion writes:
   - **Bug:** `finevsizsum` starts at `0` instead of `11`
   - Writes `peritab[0] = 10, peritab[1] = 12` — **clobbers** component 0's entries
   - `peritab[11]` and `peritab[12]` are **never written**

6. Component 9 ({11}) is ordered correctly at `peritab[13] = 11`.

7. Final `peritab`: `[10, 12, 3, 6, 1, 2, 4, 5, 7, 8, 9, ??, ??, 11]` — positions 11 and 12 are uninitialized. Vertices 0 and 13 are missing from the permutation.

8. `orderPeri` inverts `peritab` to produce `permtab`. Since vertex 13 never appears in `peritab`, `permtab[13]` is never written and retains its `0xBBBBBBBB` garbage value.

9. `SCOTCH_graphOrder` returns 0 (success).
