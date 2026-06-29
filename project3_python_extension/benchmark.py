"""
benchmark.py -- compare the pure-Python and C-extension statistics routines.

It builds one large array of pseudo-random floats, runs each implementation
several times, reports the best (lowest) wall-clock time for each, verifies
that both produce the same numerical result, and prints the speed-up factor.

Run (after building the extension with `python3 setup.py build_ext --inplace`):

    python3 benchmark.py [N] [REPEATS]

    N        number of elements in the test array (default 5_000_000)
    REPEATS  how many timed runs per implementation   (default 5)
"""

import sys
import time
import random

import stats_pure

try:
    import statsext
except ImportError:
    statsext = None


def best_time(func, data, repeats):
    """Return (result, best_seconds) over `repeats` timed runs."""
    best = float("inf")
    result = None
    for _ in range(repeats):
        start = time.perf_counter()
        result = func(data)
        elapsed = time.perf_counter() - start
        best = min(best, elapsed)
    return result, best


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 5_000_000
    repeats = int(sys.argv[2]) if len(sys.argv) > 2 else 5

    print("=" * 64)
    print(f" Statistics benchmark: N = {n:,} elements, {repeats} repeats")
    print("=" * 64)

    random.seed(42)
    data = [random.uniform(0.0, 1000.0) for _ in range(n)]

    # --- Pure Python ---
    py_result, py_time = best_time(stats_pure.compute_stats, data, repeats)
    print(f"\nPure Python : {py_time:.4f} s")
    print(f"   mean={py_result[0]:.6f} var={py_result[1]:.6f} "
          f"stddev={py_result[2]:.6f}")

    if statsext is None:
        print("\n[!] statsext not built. Run: "
              "python3 setup.py build_ext --inplace")
        return

    # --- C extension ---
    c_result, c_time = best_time(statsext.compute_stats, data, repeats)
    print(f"\nC extension : {c_time:.4f} s")
    print(f"   mean={c_result[0]:.6f} var={c_result[1]:.6f} "
          f"stddev={c_result[2]:.6f}")

    # --- Correctness check ---
    max_diff = max(abs(a - b) for a, b in zip(py_result, c_result))
    print(f"\nMax abs difference between results: {max_diff:.3e}")
    assert max_diff < 1e-6, "results diverge -- implementations disagree!"

    # --- Speed-up ---
    speedup = py_time / c_time
    print("\n" + "-" * 64)
    print(f" SPEED-UP (pure Python / C extension): {speedup:.1f}x faster")
    print("-" * 64)


if __name__ == "__main__":
    main()
