# CLAUDE.md — Scotch codebase reference

Personal mirror of [Scotch](https://gitlab.inria.fr/scotch/scotch) v7.0.11 ("Sankara"). License: CeCILL-C (French LGPL-compatible).

## Project structure

```
scotch/
  src/libscotch/       Core library (~600 .c/.h files)
  src/scotch/          CLI programs (gmap, gpart, gord, gcv, gtst, etc.)
  src/check/           Test programs (test_*.c, data/ for test graphs)
  src/libscotchmetis/  MeTiS/ParMeTiS compatibility layer
  src/esmumps/         MUMPS-compatible ordering interface
  doc/                 PDFs of manuals (user, PT-Scotch, maintenance, hands-on)
  grf/                 Sample graph/geometry files (.grf.gz)
  tgt/                 Target architecture files (.tgt)
  ci/                  GitLab CI scripts
  cmake/               CMake modules
  man/                 Man pages for CLI programs
```

## Build

```bash
mkdir build && cd build && cmake .. && make -j$(nproc)
ctest                    # run tests (157/159 pass on this mirror)
cmake -DCMAKE_BUILD_TYPE=Debug ..  # enables SCOTCH_DEBUG_ALL
```

Key CMake options: `BUILD_PTSCOTCH`, `BUILD_LIBESMUMPS`, `BUILD_LIBSCOTCHMETIS`, `BUILD_FORTRAN`, `THREADS`, `INTSIZE` (32/64), `SCOTCH_DETERMINISTIC`.

### Library targets

| Target | Description |
|---|---|
| `scotch` | Core sequential library |
| `ptscotch` | Parallel (MPI) library |
| `esmumps` / `ptesmumps` | MUMPS ordering interface |
| `scotchmetisv3` / `scotchmetisv5` | MeTiS v3/v5 compatibility |
| `ptscotchparmetisv3` | ParMeTiS v3 compatibility |
| `scotcherr` / `scotcherrexit` | Error handling (stderr / stderr+exit) |

## Return value semantics

All functions returning `int`: **0 = success, non-zero = error**.

Exceptions with richer return codes:
- `SCOTCH_graphCoarsen` / `SCOTCH_dgraphCoarsen`: 0 (success), 1 (threshold not met, not error), 2 (error)
- `SCOTCH_graphCoarsenMatch`: 0 (success), 1 (matching didn't meet threshold), 2 (error)
- `SCOTCH_archDomBipart` / `SCOTCH_archDomTerm`: 0 (success), 1 (cannot split/no terminal), 2 (error)
- `SCOTCH_graphDiamPV`: returns pseudo-diameter (positive), `SCOTCH_NUMMAX` for disconnected graphs, -1 on error
- `*Alloc` functions: return pointer or `NULL` on failure

**Return 0 means "no internal error," NOT "output is correct."** From the user manual: *"returns 0 if the graph partition has been successfully computed, and 1 else. In the latter case, the parttab array may have been partially or completely filled, but its contents are not significant."*

There is **no output validation in release builds**. In debug builds (`-DSCOTCH_DEBUG_ALL`), only ordering functions validate output via `orderCheck`. All other debug checks validate inputs or internal algorithm state. The final translation from internal domains to user `parttab[]` happens in `mapTerm()` (`mapping.c:480`) with zero validation at any debug level.

## Coding conventions

### Indentation and style
- 2-space indentation, always even columns, **never tabs**
- Declarations at block top, C89-style
- Opening brace on same line as keyword
- Always C-style `/* ... */` comments, never `//`
- `return (value);` with parentheses (treated as function call)
- Semicolons stuck to left word
- `*` and `&` stuck to right operand

### Naming conventions
- Files: `<module>_<action>[_<subname>].[ch]` (e.g., `bgraph_bipart_fm.c`)
- Functions: `<ClassName><MethodName>` (e.g., `kgraphMapFm`)
- Radicals: `vert` (vertex), `edge` (arc), `velo` (vertex load), `edlo` (arc load), `graf` (graph), `arch` (architecture)
- Suffixes: `nbr` (count), `nnd` (count + baseval), `num` (index), `val` (value), `tab` (array), `tax` (based array = tab - baseval), `ptr` (pointer), `sum`, `bas` (base start)
- Prefixes: `src` (source), `coar` (coarse), `fine`, `org` (original), `ind` (induced), `loc` (local), `glb` (global)
- Standard methods: `Init`, `Exit`, `Free`, `Alloc`, `Check`, `Load`, `Save`, `Copy`

### Based arrays (critical pattern)
- `baseval` is 0 or 1 (supports C and Fortran indexing)
- `*tax = *tab - baseval` so `tax[baseval]` always addresses first element
- `*nnd = *nbr + baseval` for end-of-range
- Loop pattern: `for (xxxnum = baseval; xxxnum < xxxnnd; xxxnum++)`
- Memory alloc/free always uses `*tab` pointers, never `*tax`

### Section headers
```c
/**************************************/
/*                                    */
/* The consistency checking routines. */
/*                                    */
/**************************************/
```

## Internal class hierarchy

| Module | Structure | Purpose |
|---|---|---|
| `graph` | `Graph` | Base source graph (adjacency lists) |
| `bgraph` | `Bgraph` | Graph + edge bipartition state |
| `vgraph` | `Vgraph` | Graph + vertex separator state |
| `kgraph` | `Kgraph` | Graph + k-way mapping state |
| `wgraph` | `Wgraph` | Graph + overlap partition state |
| `hgraph` | `Hgraph` | Graph + halo vertices (for ordering) |
| `hmesh` | `Hmesh` | Mesh + halo (for ordering) |
| `mapping` | `Mapping` | Maps vertices to architecture domains |
| `order` | `Order` | Fill-minimizing block ordering (tree of `OrderCblk`) |
| `arch` | `Arch` | Target architecture |
| `dgraph` | `Dgraph` | Distributed graph (MPI) |
| `bdgraph` | `Bdgraph` | Distributed bipartition graph |
| `vdgraph` | `Vdgraph` | Distributed separator graph |

### Key internal structures

**Kgraph** (k-way mapping): Contains `Graph s`, current `Mapping m` (never incomplete — all `parttax` cells non-negative), old `Mapping r.m` (may have -1 for incomplete). Domain array `domntab` ownership transfers between parent/derived Kgraphs by pointer.

**Mapping**: `parttax` (based, indexes into `domntab`), `domntab` (un-based array of `ArchDom`), `grafptr`, `archptr`. Fixed-vertex domains placed first in `domntab`. Flags: `MAPPINGFREEDOMN`, `MAPPINGFREEPART`, `MAPPINGINCOMPLETE`.

**Order**: `peritab` (inverse permutation), tree of `OrderCblk` nodes. Types: `ORDERCBLKLEAF`, `ORDERCBLKNEDI` (nested dissection — separator is always last sub-block), `ORDERCBLKDICO` (disconnected components), `ORDERCBLKSEQU` (sequential).

## Debug flag hierarchy

Defined in `src/libscotch/module.h`:

| Level | Flag | Enabled by | What it checks |
|---|---|---|---|
| 1 | `SCOTCH_DEBUG` | `SCOTCH_DEBUG_ALL` | Input parameter validation, structure sizes, null checks |
| 2 | `SCOTCH_DEBUG_ALL` | `CMAKE_BUILD_TYPE=Debug` | `graphCheck` on inputs, `orderCheck` on ordering output, internal consistency |
| 3 | `SCOTCH_DEBUG_FULL` | Manual | Expensive per-iteration checks. **Stripped before releases** (tagged with `BROL` keyword) |

Per-module flags: `SCOTCH_DEBUG_<MODULE><LEVEL>` (e.g., `SCOTCH_DEBUG_KGRAPH2`). Every class must have a `Check` method, and every function that creates/updates objects must call it in debug mode. Check routines live in `*_check.c` files.

### Check routines (19 files in src/libscotch/)

`graph_check.c`, `bgraph_check.c`, `vgraph_check.c`, `kgraph_check.c`, `hgraph_check.c`, `wgraph_check.c`, `mesh_check.c`, `hmesh_check.c`, `vmesh_check.c`, `order_check.c`, `mapping_check.c`, `dgraph_check.c`, `bdgraph_check.c`, `vdgraph_check.c`, `hdgraph_check.c`, `dgraph_match_check.c`, `dorder_check.c`, `library_graph_check.c`, `library_dgraph_check.c`

## Partition and mapping output

Partition values are always **0-based**: *"target vertices are numbered from 0 to partnbr−1"* (complying with MPI rank conventions). This applies to both sequential and distributed functions.

For variable-sized architectures (clustering), clusters are labeled with a binary scheme: root=1, children of cluster i are 2i and 2i+1.

## Known issues on this mirror

### MeTiS base-1 partition bug
`metis_graph_part.c:318-323` adds `baseval` to partition values when `numflag != 0`, producing values in `[1, nparts]` instead of `[0, nparts-1]`. This contradicts both the Scotch docs and the real MeTiS API where `numflag` only affects graph array indexing. Causes `test_libmetisv3_2` and `test_libmetisv5_2` to fail.

### graphColor
The user manual says coloring is "not guaranteed to be maximal" but does NOT explicitly guarantee it produces a valid proper coloring. v7.0.11 includes a fix for a bug where sequential coloring didn't consider neighbors colored in the same pass (`e0a90c7`).

## Public validation API

| Routine | Validates | Purpose |
|---|---|---|
| `SCOTCH_graphCheck` | Graph structure | Input validation after `graphBuild` |
| `SCOTCH_graphOrderCheck` | Ordering | **Only public output validator** |
| `SCOTCH_meshCheck` | Mesh structure | Input validation after `meshBuild` |
| `SCOTCH_dgraphCheck` | Distributed graph | Input validation after `dgraphBuild` |
| `SCOTCH_meshOrderCheck` | Mesh ordering | Output validation for mesh orderings |

No public `SCOTCH_graphMapCheck` or `SCOTCH_graphColorCheck` exists.

## Test suite

Tests in `src/check/`, registered in `src/check/CMakeLists.txt`. Sequential tests link against `scotch`, parallel tests against `ptscotch` (run via `mpiexec -n 3`). Test data in `src/check/data/`.

This mirror adds output validity checks to 23 test files (+523 lines): mathematical property checks on partitions/orderings/colorings, `graphCheck`/`dgraphCheck` on operation outputs, architecture roundtrip consistency, and return code checks on strategy builders.

## MeTiS compatibility layer

Located in `src/libscotchmetis/`. Built as `scotchmetisv3` (v3 API) and `scotchmetisv5` (v5 API) from same sources with `SCOTCH_METIS_VERSION=3|5`. `SCOTCH_METIS_PREFIX` option prefixes symbols with `SCOTCH_` to avoid conflicts.

Key limitations:
- `METIS_OPTION_NUMBERING` is the only option supported
- Only first constraint used for multi-constraint partitioning
- `METIS_PartGraphRecursive` has extra edge-cut recomputation cost (processes full graph)
- `METIS_PartGraphVKway` approximates communication volume via edge weights
- `METIS_MeshToDual` output arrays are malloc'd — user must `free()` them
- The `part` output array is documented as having "same meaning as parttab" (0-based), but the implementation incorrectly adds `baseval` for base-1 graphs
- MeTiS v3 compatibility won't work if `SCOTCH_Num` is coerced to non-standard sizes

### ParMeTiS compatibility
Library: `ptscotchparmetisv3`. Link order: `-lptscotchparmetis` BEFORE `-lparmetis` and `-lptscotch`. Only V3 stubs provided. `ParMETIS_V3_PartGeomKway` ignores geometry (calls `PartKway` directly). `ParMETIS_V3_NodeND` works on non-power-of-2 process counts (unlike real ParMeTiS), but `sizes` array filled with -1 in that case.

## PT-Scotch (distributed) specifics

### MPI communicator rules
- `dgraphInit` does NOT duplicate the communicator — keeps a reference
- User manages communicator lifetime (free AFTER `dgraphExit`)
- Use `MPI_Comm_dup` when operating on multiple graphs in parallel
- Strategy strings must be identical on all processes

### Critical gotchas
- `dgraphBuild` does NOT copy arrays — references them. Don't modify while in use.
- Optional arrays: ALL processes must provide them or NONE (mixing causes deadlocks)
- Never include `scotch.h` with `libptscotch.a` — structure sizes differ
- `dgraphBand` clobbers `seedloctab` (used as internal queue)
- Band/induced/coarsened graphs may have highly uneven vertex distribution
- v6.0/7.0 limitation: cannot map distributed graphs onto non-complete architectures
- `SCOTCH_Dmesh` is a prototype with no official support

### Environment variables
- `SCOTCH_PTHREAD_NUMBER`: max threads (-1 = all cores)
- `SCOTCH_DETERMINISTIC`: 0 (faster, non-deterministic) / 1 (deterministic)
- `SCOTCH_RANDOM_FIXED_SEED`: 0 (dynamic seed) / 1 (fixed seed)

## Strategy strings

Strategies are parsed at runtime via Flex/Bison. Grammar: `|` (best-of selection), space (sequence), `()` (grouping), `/cond?strat1[:strat2];` (conditional).

Condition variables: `edge`, `levl`, `load`, `load0`, `mdeg`, `proc`, `rank`, `vert`.

Use `SCOTCH_stratInit` + direct use (auto-fills defaults) or builders like `SCOTCH_stratGraphMapBuild`. Strategy flags: `SCOTCH_STRATDEFAULT`, `SCOTCH_STRATBALANCE`, `SCOTCH_STRATQUALITY`, `SCOTCH_STRATSAFETY`, `SCOTCH_STRATSPEED`.

**Strategy creation routines are NOT guaranteed reentrant** (third-party lexical analyzers). Protect with mutex if called from multiple threads.

### Sequential methods
- **Mapping**: `m` (multi-level), `r` (recursive bipartitioning), `b` (band), `d` (diffusion, band-graph only), `f` (Fiduccia-Mattheyses), `h` (greedy), `x` (exactifier/rebalancer)
- **Bipartitioning**: same method letters applied to 2-way case
- **Ordering**: `n` (nested dissection), `e` (minimum degree), `s` (simple/natural), `q` (sequential from parallel)
- **Separation**: same letters as bipartitioning
- **Overlap partitioning**: `g` (greedy growing), `b` (band), `m` (multi-level), `z` (zero)

### Parallel methods
- **Mapping**: `r` (recursive bipartitioning with `seq=` and `sep=` sub-strategies)
- **Bipartitioning**: `b` (band), `d` (diffusion), `m` (multi-level), `q` (multi-sequential), `x` (rebalancer), `z` (zero)
- **Ordering**: `n` (nested dissection), `q` (sequential), `s` (simple)

## File formats

### Graph files (.grf)
Line 1: version (0). Line 2: vertnbr, edgenbr. Line 3: baseval, numeric flag (3 digits: vertex weights / edge weights / vertex labels). Then vertnbr lines: [label] [weight] degree neighbor1 [edgeweight1] ...

### Distributed graph files (.dgr)
One file per process. Named with `%p` (proc count) and `%r` (proc rank). Version 2. Contains global and local counts plus local adjacency data.

### Architecture files (.tgt)
Decomposition-defined or algorithmically-coded. Types include: `cmplt` (complete), `cmpltw` (weighted complete), `hcub` (hypercube), `mesh2D`/`mesh3D`/`meshXD`, `torus2D`/`torus3D`/`torusX`, `tleaf` (tree-leaf), `ltleaf` (labeled tree-leaf), `sub` (sub-architecture).

## Header generation

Public headers (`scotch.h`, `ptscotch.h`, `metis.h`, `parmetis.h`) are generated from templates (`library.h`, `library_pt.h`, `library_metis.h`, `library_parmetis.h`) by the `dummysizes` program, which replaces placeholder sizes with actual computed struct sizes for the target platform.
