# Cache Hierarchy & Branch Predictor Simulator

A reproducible **trace-driven C++17 architecture-modeling project** with a configurable three-level cache lookup hierarchy, LRU/tree-PLRU policies, bimodal/gshare/tournament branch predictors, a deterministic trace generator, correctness tests, configuration sweeps, plots, and a self-contained Kaggle CPU notebook.

## Quick start (Linux / Kaggle CPU)

```bash
g++ -O2 -std=c++17 -Wall -Wextra src/main.cpp -o simulator
python3 -m unittest discover -s tests -v
python3 scripts/generate_trace.py --output traces/synthetic.trace --accesses 50000 --seed 42
./simulator --trace traces/synthetic.trace --out results/base.csv
python3 scripts/sweep.py --sim ./simulator --trace traces/synthetic.trace --out results/sweep.csv
python3 scripts/plot.py
```

Requirements: GCC or Clang with C++17, Python 3; Matplotlib only for visualizations. **No NVIDIA GPU required.** Kaggle users may import `Kaggle_Final.ipynb` and select Accelerator: None.

## Model and semantics

- Memory trace record `M <address>` checks L1 -> L2 -> L3 until a hit, then fills the previously missed levels (or all levels on total miss). All levels use the same configurable block size, associativity and replacement policy, with independent capacities. Address arithmetic uses unsigned 64-bit values.
- **This is a simplified serial hierarchy, not a strictly inclusive cache implementation.** It does not model eviction-triggered invalidation, dirty/writeback traffic, prefetching, coherence, cache timing, CPU pipeline stalls, or CPI.
- LRU tracks last access recency; tree-PLRU requires power-of-two ways. Hits update policy state; fills select invalid entries before eviction.
- Bimodal/gshare use 2-bit saturating counters initialized weak-not-taken. Gshare uses global outcome history; tournament chooses between bimodal and gshare using 2-bit chooser counters.
- Local miss rate is misses/requests **at each cache level**, not the fraction of all memory accesses. MPKI is 1000 × misses / dynamic instructions. For trace records without `I`, an estimated count (`M+B`) is used and the output flags `instruction_count_estimated=1`; it is **not a measured instruction count**.
- `I <count>` is an external count supplied by a trace author; the generator writes an illustrative synthetic instruction count, not a measured count from an executable.

Trace example:
```
I 10000
M 0x1000
M 0x1040
B 0x400 T
B 0x400 N
```

## Verification and reproducibility

`tests/test_sim.py` includes directed tests for cold hits/misses, LRU eviction, PLRU behavior, branch-counter training, miss-level refill, parsing errors, invalid geometry, empty traces and a deterministic randomized LRU cross-check against a separate Python oracle. The independent oracle checks the L1 cache for a single-set two-way configuration. These tests are **not formal verification** and do not validate real CPU timing.

The included trace is **synthetic** (seed 42), not SPEC CPU or a hardware execution trace. `results/` contains outputs produced by the commands above. Change generator seed and working-set patterns to explore sensitivity; do not attribute any measured miss-rate improvement to real applications without additional trace evidence.

## Results & interpretation

The suite runs 21 replacement/associativity/predictor combinations on the synthetic trace. See `results/base.csv`, `results/sweep.csv`, and PNG plots. Cache local miss rates can be non-monotonic as configurations alter the stream reaching lower levels. Predictor accuracy depends on the generated branches; it is not a universal accuracy figure.

## NVIDIA relevance

Demonstrates C++ functional architecture modelling, address decomposition, policy trade-offs, trace processing, reproducible Python automation and validation. **It does not demonstrate RTL/SystemVerilog, ARM verification, ASIC design, or cycle-accurate performance estimation.**

## Resume bullets (only after reproducing tests/results)

- Developed a trace-driven **C++17** three-level cache simulator with configurable size, associativity and **LRU/tree-PLRU** replacement; measured per-level miss rates and MPKI.
- Implemented **bimodal, gshare and tournament** branch predictors with 2-bit saturating counters; automated **21 architecture configurations** using Python and generated comparative plots.
- Validated cache behavior with directed tests and an independently implemented randomized **LRU reference model**; documented simplifying assumptions and repeatable synthetic workloads.

## License

MIT — see `LICENSE`.
