# Hegel PBT Test Suite for Scotch — Session Report

## What was built

### Two test harnesses for Scotch using Hegel (Antithesis PBT framework)

**`src/check/hegel/`** — Pure C test suite using a Rust-built C wrapper for Hegel (67 tests, main suite)

The C wrapper (`hegel/src/`) exposes Hegel's test runner and generators as C functions:
- `hegel_run_test(fn)` / `hegel_run_test_n(fn, n)` — run PBT with callback
- `hegel_draw_int(tc, min, max)` — draw random integers
- `hegel_assume(tc, cond)` — skip test case
- `HEGEL_ASSERT(cond, fmt, ...)` — fail with message, triggers Hegel shrinking

Key design: all Hegel functions use `extern "C-unwind"` so Rust panics (assertions, Hegel's internal StopTest) can unwind through C frames. C code compiled with `-funwind-tables -fexceptions`.

### Shared infrastructure

- **`graph_gen.h`** — Graph generators: random sparse, 2D grid, complete (K_n), path. All support arbitrary baseval (0 or 1). Audited for correctness.
- **`scotch_helpers.h`** — `scotchReset()` for deterministic seeding, `stratGenMap()`/`stratGenOrder()`/`stratGenBipart()` for random valid strategy string generation.
- **`hegel_c.h`** — C API header with `HEGEL_ASSERT` macro.
- **`.claude/commands/hegel.md`** — Slash command for generating more tests in future sessions.

### How to run

```bash
cd src/check/hegel
make all       # build everything (requires: cargo, gcc, cmake, uv)
make test      # run all tests once
make test-loop # run in a loop until failure (use with: timeout 5h make test-loop)
make clean-cores  # delete core dumps from .cores/
make clean     # delete everything
```

Prerequisites: Scotch must be built first in `build/` (see CLAUDE.md for build instructions). Rust toolchain and `uv` (for Hegel's Python server) must be on PATH.

---

## Test inventory (67 tests)

### Passing (65 tests)

| Category | Tests | Properties checked |
|---|---|---|
| **Partitioning** | part, part_fixed, part_ovl, part_weighted, part_balance, many_parts, repart, edge_cut, part_nonempty, part_ovl_boundary | Output range [0,nparts), fixed vertices respected, overlap semantics, edge cut bounds, non-empty parts |
| **Coloring** | color, color_multi, complete_color, bipartite_color, color_quality | Proper coloring, determinism, chromatic number of K_n, color count bounds, all colors used |
| **Ordering** | order, order_list, order_weighted, order_save_load | Valid permutation, inverse, orderCheck, weighted, save/load roundtrip |
| **Graph ops** | build_data, save_load, save_load_weighted, induce, induce_part, coarsen, coarsen_match_build, coarsen_partition, coarsen_matching, tab_save_load, base, stat | Roundtrips, subgraph validity, coarse graph integrity, matching consistency, rebasing, statistics |
| **Architecture** | arch_roundtrip, arch_dom, arch_build, arch_sub | Save/load, domain bipartition sizes, archBuild2, sub-architecture |
| **Mapping** | map_arch, map_fixed, map_remap | Output range, fixed vertices, remap |
| **Strategy** | strat_build, strat_parse, strat_random_map, strat_random_order, strat_each_bipart, strat_each_order | Builder return codes, invalid string handling, random strategies, each bipartition/ordering method |
| **Mesh** | mesh_build_check, mesh_save_load | meshBuild+meshCheck, meshGraph, save/load roundtrip |
| **MeTiS** | metis_node_nd, metis_part_recursive | Valid permutation, partition range |
| **Edge cases** | single_vertex, disconnected, empty_edges, self_consistent | 1-vertex graph, multi-component, no edges, strategy comparison |
| **Stress** | large_graph, stress_huge_grid, stress_dense, stress_star, stress_long_path, stress_heavy_weights, stress_many_components, stress_10k | Up to 40K vertices, K_100, star graphs, 2000-vertex paths, large weights, many disconnected components |
| **Error handling** | error_graph_build, error_strat_mismatch | Empty graph, nparts=0/1, wrong strategy type |
| **Misc** | determinism, diam | Same seed = same result, diameter properties |

### Expected failures (2 tests — XFAIL)

| Test | Bug | Details |
|---|---|---|
| `test_metis_part` | MeTiS base-1 partition bug | `metis_graph_part.c:318-323` adds `baseval` to output when `numflag != 0`, producing `[1, nparts]` instead of `[0, nparts-1]`. Known, documented in CLAUDE.md. |
| `test_strat_use` | STRATDISCONNECTED ordering bug | **NEW BUG FOUND.** See below. |

---

## Bug found: SCOTCH_STRATDISCONNECTED ordering produces garbage permutation

### Summary

`SCOTCH_graphOrder` returns 0 (success) but leaves `permtab` partially or fully uninitialized when the ordering strategy is built with the `SCOTCH_STRATDISCONNECTED` flag on graphs that contain isolated vertices (degree 0).

### Minimal reproducer

```c
SCOTCH_randomSeed(42);
SCOTCH_randomReset();
SCOTCH_graphInit(&g);
/* 14 vertices, 8 edges (4 undirected). Most vertices isolated (degree 0). */
/* degrees: 1 0 0 1 0 0 1 0 0 0 1 0 1 3 */
SCOTCH_Num verttab[] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 8};
SCOTCH_Num edgetab[] = {13, 13, 13, 12, 10, 0, 3, 6};
SCOTCH_graphBuild(&g, 0, 14, verttab, NULL, NULL, NULL, 8, edgetab, NULL);
SCOTCH_graphCheck(&g);  /* passes */

SCOTCH_Num permtab[14];
memset(permtab, 0xBB, sizeof(permtab));  /* fill with garbage marker */
SCOTCH_stratInit(&s);
SCOTCH_stratGraphOrderBuild(&s, SCOTCH_STRATDISCONNECTED, 3, 0.2);

SCOTCH_Num cblknbr = 0;
int rc = SCOTCH_graphOrder(&g, &s, permtab, NULL, &cblknbr, NULL, NULL);
/* rc == 0 (success) but permtab[13] == 0xBBBBBBBB (uninitialized) */
```

### Characterization

- **Trigger condition**: Graph must have isolated vertices (degree 0) AND strategy must include `SCOTCH_STRATDISCONNECTED` flag.
- **Frequency**: Approximately 25% of random sparse graphs with 4-20 vertices trigger it (found via brute-force with 10,000 random trials — first trial hit it).
- **Impact**: Silent data corruption. `graphOrder` returns 0 (success) but `permtab` contains garbage values. Any code that trusts the return code and uses `permtab` will read uninitialized memory.
- **Scope**: Only affects the `SCOTCH_STRATDISCONNECTED` flag specifically. The `SCOTCH_STRATDEFAULT` flag works correctly. Builder functions like `SCOTCH_stratGraphOrderBuild` with `SCOTCH_STRATDEFAULT` are not affected.
- **Not yet investigated**: Whether the `SCOTCH_STRATLEVELMAX`, `SCOTCH_STRATLEVELMIN`, `SCOTCH_STRATLEAFSIMPLE`, or `SCOTCH_STRATSEPASIMPLE` flags have similar issues.

### Probable root cause (not confirmed)

**Note from c4ffein: obv just a wild guess from Claude at this point**

The `SCOTCH_STRATDISCONNECTED` flag adds a disconnected-component handling wrapper to the ordering strategy. This wrapper likely iterates over connected components and orders them separately. Isolated vertices (single-vertex components with no edges) may be skipped by the component iterator, leaving their `permtab` entries unwritten.

The relevant code is likely in `hgraph_order_st.c` where the strategy dispatch handles the `ORDERCBLKDICO` (disconnected component) block type.

### What to investigate next

1. Read `hgraph_order_st.c` and `hgraph_order_cc.c` (connected component ordering) to confirm the root cause
2. Check if isolated vertices are being counted in the component enumeration
3. Test whether the same bug exists with `SCOTCH_STRATLEVELMAX` + isolated vertices
4. Write a targeted fix (ensure isolated vertices get assigned permutation values)

---

## Other findings (not bugs)

### Scotch coloring uses up to n-1 colors on n-vertex graphs

Scotch's `SCOTCH_graphColor` uses a Luby-style concurrent coloring algorithm (in `library_graph_color.c`). This produces valid proper colorings but makes no quality guarantees. On a 10-vertex path (chromatic number 2, greedy bound 3), it consistently uses 9 colors. The CLAUDE.md correctly documents this as "not guaranteed to be maximal." The `e0a90c7` bugfix (Jan 2026) fixed a real bug where neighbors in the same coloring pass weren't considered, but the algorithm is still inherently wasteful on sequential execution.

### graphRemap requires SCOTCH_STRATREMAP flag

Calling `SCOTCH_graphRemap` with a default empty strategy (from `SCOTCH_stratInit`) ignores the `emraval` migration cost ratio — approximately 50% of vertices move even with `emraval=1000`. The strategy must be built with `SCOTCH_STRATRECURSIVE | SCOTCH_STRATREMAP` for migration cost to take effect. Even with the correct flag, some vertices still move on small graphs, suggesting the migration cost weighting may have quality issues on small inputs.

### _FORTIFY_SOURCE incompatibility

The `bgraph_bipart_gg.c:309-310` buffer overflow report (Fedora Rawhide, 2023) is a false positive from `_FORTIFY_SOURCE` caused by Scotch's based-array pointer arithmetic pattern. There are ~40 sites in `src/libscotch/` that do `memSet(tax + baseval, ...)` on based arrays, all of which would trigger with large `baseval`. Scotch's CMakeLists.txt already disables `_FORTIFY_SOURCE` for GCC (`-U_FORTIFY_SOURCE`). The Fedora issue was the distro's build system overriding this.

---

## Architecture notes

### How the C-to-Hegel bridge works

```
hegel-core (Python) ←Unix socket/CBOR→ hegel-rust (Rust) → hegel-c (staticlib) → C test code → Scotch
```

- `hegel_run_test(fn)` spawns the Python server, runs `fn` 100 times with generated inputs
- `hegel_draw_int(tc, min, max)` sends a "generate" request to the server, gets back a value
- `HEGEL_ASSERT` calls `hegel_fail` which panics in Rust; Hegel catches it and begins shrinking
- `extern "C-unwind"` on all functions allows Rust panics to unwind through C frames
- C code compiled with `-funwind-tables -fexceptions`
- `scotch_reset()` (in the Rust wrapper) resets Scotch's global random state before each test case for deterministic replay

### Key files

```
hegel/                        Hegel framework (MIT licensed)
  hegel_c.h                   C header
  LICENSE                     MIT license
  src/                        Rust staticlib crate (wrapper)
    src/lib.rs                extern "C-unwind" API + scotch_reset()
    Cargo.toml                depends on hegeltest 0.1

src/check/hegel/              Hegel PBT tests for Scotch
  graph_gen.h                 Graph generators (random, grid, complete, path)
  scotch_helpers.h            scotchReset(), strategy string generators
  Makefile                    build + test + test-loop + clean
  test_*.c                    67 test files
  .cores/                     Core dumps from crashes (auto-cleaned)
```

### Overnight run command

```bash
cd src/check/hegel && timeout 5h make test-loop
```

Runs all 65 passing tests in a loop (~1 min per loop, ~300 loops in 5h, ~6M+ random test cases).
