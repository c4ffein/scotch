SCOTCH_INC   = ../../../build/src/include
SCOTCH_LIB   = ../../../build/lib
HEGEL_DIR    = ../../../hegel
HEGEL_C_LIB  = $(HEGEL_DIR)/src/target/release

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -I$(SCOTCH_INC) -I. -I$(HEGEL_DIR) -funwind-tables -fexceptions
LDFLAGS  = -L$(SCOTCH_LIB) -L$(HEGEL_C_LIB) \
           -lhegel_c -lscotch -lscotcherr \
           -lpthread -lm -ldl
LDFLAGS_METIS = -L$(SCOTCH_LIB) -L$(HEGEL_C_LIB) \
                -lhegel_c -lscotchmetisv3 -lscotch -lscotcherr \
                -lpthread -lm -ldl

TESTS = test_graph_part test_graph_color test_graph_order \
        test_graph_build_data test_graph_induce test_graph_coarsen \
        test_arch_roundtrip test_strat_build \
        test_graph_save_load test_determinism \
        test_graph_diam test_graph_map_arch \
        test_graph_part_weighted test_graph_base test_graph_part_ovl \
        test_graph_part_fixed test_graph_induce_part test_graph_repart \
        test_edge_cut test_compose_part_induce test_arch_dom \
        test_graph_stat test_graph_coarsen_match_build \
        test_metis_node_nd \
        test_graph_order_list test_graph_map_fixed \
        test_graph_color_multi \
        test_graph_coarsen_partition test_graph_order_weighted \
        test_graph_part_balance test_graph_save_load_weighted \
        test_graph_disconnected test_graph_single_vertex \
        test_graph_self_consistent test_graph_map_remap \
        test_graph_many_parts test_graph_complete_color \
        test_graph_bipartite_color test_graph_empty_edges \
        test_arch_build test_arch_sub \
        test_mesh_build_check test_mesh_save_load \
        test_strat_parse test_large_graph \
        test_stress_huge_grid test_stress_dense \
        test_stress_star test_stress_long_path \
        test_stress_heavy_weights test_stress_many_components \
        test_error_graph_build test_error_strat_mismatch \
        test_graph_color_quality test_graph_coarsen_matching \
        test_graph_part_ovl_boundary test_graph_part_nonempty \
        test_strat_random_map test_strat_random_order \
        test_strat_each_bipart test_strat_each_order \
        test_stress_10k \
        test_graph_order_save_load test_graph_tab_save_load \
        test_metis_part_recursive \
        test_strat_use

# test_metis_part exposes a known bug (CLAUDE.md: MeTiS base-1 partition bug)
TESTS_XFAIL = test_metis_part

.PHONY: all clean clean-cores test test-loop hegel-c-lib bench bench_fork bench_nofork bench_nofork_bins

all: hegel-c-lib $(TESTS) $(TESTS_XFAIL)

hegel-c-lib:
	cd $(HEGEL_DIR)/src && cargo build --release

test_metis_part: test_metis_part.c hegel-c-lib graph_gen.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_METIS)

test_metis_node_nd: test_metis_node_nd.c hegel-c-lib graph_gen.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_METIS)

test_metis_part_recursive: test_metis_part_recursive.c hegel-c-lib graph_gen.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_METIS)

test_%: test_%.c hegel-c-lib graph_gen.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@mkdir -p .cores; \
	for t in $(TESTS); do \
		echo "=== $$t ==="; \
		./$$t || exit 1; \
	done
	@mv -f core.* .cores/ 2>/dev/null; true
	@ulimit -c 0; \
	for t in $(TESTS_XFAIL); do \
		echo "=== $$t (expected failure) ==="; \
		if ./$$t > /dev/null 2>&1; then \
			echo "UNEXPECTED PASS — bug may be fixed!"; \
		else \
			echo "XFAIL (known bug)"; \
		fi; \
	done
	@echo "All tests passed."

test-loop: all
	@echo "Running test suite in a loop until failure or timeout..."
	@echo "Start: $$(date)"
	@run=1; \
	while true; do \
		echo ""; \
		echo "======== Run $$run ($$(date)) ========"; \
		mkdir -p .cores; \
		for t in $(TESTS); do \
			./$$t > /dev/null 2>&1; \
			if [ $$? -ne 0 ]; then \
				echo "FAIL: $$t (run $$run)"; \
				mv -f core.* .cores/ 2>/dev/null; true; \
				exit 1; \
			fi; \
		done; \
		mv -f core.* .cores/ 2>/dev/null; true; \
		echo "Run $$run: all 66 tests passed"; \
		run=$$((run + 1)); \
	done

BENCH_TESTS = test_graph_part test_stress_10k

bench: all bench_fork bench_nofork

bench_fork: all
	@echo "=== fork mode (default, crash-safe) ==="
	@for t in $(BENCH_TESTS); do \
		printf "  %-25s" "$$t:"; \
		start=$$(date +%s%N); \
		for i in $$(seq 1 5); do \
			./$$t > /dev/null 2>&1; \
		done; \
		end=$$(date +%s%N); \
		elapsed=$$(( (end - start) / 1000000 )); \
		printf "%5dms total  %5dms avg (5 runs)\n" $$elapsed $$(( elapsed / 5 )); \
	done

bench_nofork: all bench_nofork_bins
	@echo "=== nofork mode (no crash isolation) ==="
	@for t in $(BENCH_TESTS); do \
		printf "  %-25s" "$$t:"; \
		start=$$(date +%s%N); \
		for i in $$(seq 1 5); do \
			./bench_$$t > /dev/null 2>&1; \
		done; \
		end=$$(date +%s%N); \
		elapsed=$$(( (end - start) / 1000000 )); \
		printf "%5dms total  %5dms avg (5 runs)\n" $$elapsed $$(( elapsed / 5 )); \
	done

bench_nofork_bins:
	@for t in $(BENCH_TESTS); do \
		$(CC) $(CFLAGS) -DHEGEL_BENCH_NOFORK -o bench_$$t $$t.c $(LDFLAGS); \
	done

clean-cores:
	rm -rf .cores/
	rm -f core.*

clean:
	rm -f $(TESTS) $(TESTS_XFAIL) bench_*
	rm -rf .cores/
	rm -f core.*
	cd $(HEGEL_DIR)/src && cargo clean
