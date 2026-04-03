# Hegel C binding

Currently a duct-taped C library — your tests include a .h, link a .a, never see Rust.
- The current implementation is Rust because that's what talks to the Hegel server (which is a Rust/Python service).
- The alternative would be reimplementing the Hegel wire protocol in pure C.
- This will only be done once there is a sufficient test suite to verify consistency between these 2 options.
- While the Rust bridge was the fast path to get everything working, it will be kept - we want both:
  - a future pure C implem connecting to the worker through the socket
  - a version as close to the the official Rust implementation as possible
    - if the Hegel team ever release a low-level C header for FFI bindings, we'll adapt to it, still providing a nice standard layer to make it as adapted to a C codebase as possible

## TODO
- [ ] Suppress "Draw N:" trace output from Hegel during shrinking (noisy)
- [ ] Create specific repo (could get Scotch through scripts for real-world tests?)
- [ ] Full test suite, verify our sloppy duct-taped layer can handle tests
  - [ ] Grammar-based strategy fuzzer
    — recursive generator for structured strings with
      - method letters,
      - parameter ranges,
      - composition (`A B`),
      - best-of (`A | B`),
      - and conditionals (`/vert > N ? A : B ;`)
    - Scotch strategy strings are an excellent real-world test case for this: they have
      - a well-defined grammar,
      - three method tables (mapping: 7 methods, separation: 8, ordering: 9),
      - and the parser rejects invalid strings deterministically
        — making them ideal for validating hegel-c's generator composability end-to-end. 
    - See `scotch_helpers.h` for the current fixed-menu approach.
- [ ] CI integration (GitHub CI script in `ci/`)
- [ ] verify Hegel's database replay failing cases across runs with fork mode
- [ ] PT-Scotch (MPI) tests — fork mode might actually work if we're clever about it. The naive problem: `MPI_Init` in `main()` runs before `hegel_run_test`, so forked children inherit stale MPI state. But we fork before the test body runs, so in theory the test function could call `MPI_Init`/`MPI_Finalize` itself, inside the child. The tricky part: `mpiexec -n 3` means 3 parent processes each fork a child, and those 3 children need to `MPI_Init` together — requires a cross-rank barrier before the fork so children are synchronized. Just musings for now, needs a prototype to see if MPI implementations actually tolerate this. Fallback: nofork mode works fine (no crash isolation but shrinking still works) => tbh maybe slop reflection from Claude, maybe real, won't check today
  - [ ] Just verify by writing tests of hegel-c tests handling various mpi code
- [ ] Pool hegel server across test cases — startup cost (~1s) dominates short tests => told Claude, verify
- [ ] Parallel test execution
- [ ] Look at `graph_gen.h` / `scotch_helpers.h` for inspiration — some data structures (CSR graph builders, strategy string generators) could still generalize into reusable hegel-c helpers
- [ ] Real C implementation.
  - [ ] Compare with existing C/Rust bridge implementation using PBT

## Design decisions
- **Pure C, no C++.** A separate C++ Hegel binding is WIP by the Hegel team. This lib stays pure C.
- **`graph_gen.h` / `scotch_helpers.h` stay in the Scotch test harness**, not in hegel-c. They're Scotch-specific. But they contain patterns (CSR builders, strategy generators) worth generalizing later.

## Benchmarking

### Fork vs nofork (this project)

```
make bench
```

Runs each of `test_graph_part` (simple) and `test_stress_10k` (heavy) in
both fork and nofork modes, 5 times each. Typical results:

| Test           | Fork    | Nofork  | Overhead |
|----------------|---------|---------|----------|
| graph_part     | ~6.7s   | ~5.6s   | ~18%     |
| stress_10k     | ~2.6s   | ~2.5s   | ~2%      |

The overhead is dominated by Hegel's server startup (~1s), not fork itself.
For tests where the actual work is non-trivial, fork overhead is negligible.

### How to add your own bench tests

Add the test name to `BENCH_TESTS` in the Makefile:

```makefile
BENCH_TESTS = test_graph_part test_stress_10k test_your_new_test
```

### Benchmarking against other codebases

To benchmark Hegel's C binding on a different C library:

1. **Write a test** that follows the pattern in `test_graph_part.c`:
   - `#include "hegel_c.h"`
   - A test function `void myTest(hegel_testcase *tc)` that uses `hegel_draw_*` and `HEGEL_ASSERT`
   - `main()` calls `hegel_run_test(myTest)`

2. **Link against** `-lhegel_c` (from `hegel/src/target/release/`) plus your library

3. **Run both modes:**
   ```bash
   # Fork mode (default)
   gcc -O2 -I/path/to/hegel -o test_fork my_test.c -L/path/to/hegel_c -lhegel_c -lpthread -lm -ldl
   time ./test_fork

   # Nofork mode
   gcc -O2 -DHEGEL_BENCH_NOFORK -I/path/to/hegel -o test_nofork my_test.c -L/path/to/hegel_c -lhegel_c -lpthread -lm -ldl
   time ./test_nofork
   ```

4. **What to compare:**
   - Per-run wall time (includes Hegel server startup, ~1s fixed cost)
   - For apples-to-apples, use `hegel_run_test_n` with large N (e.g., 1000) so
     the server startup is amortized and you're measuring per-case fork overhead
   - The fork overhead per case is ~50-100µs on Linux (COW page table copy).
     If your test case does >1ms of work, fork is free.
