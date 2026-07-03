# Project 3 - Building a Python Performance Extension

**Goal:** a data-analysis program computing statistics over large arrays is too
slow in pure Python. Re-implement the hot loop as a **CPython C extension** and
compare the two.

**Workload:** compute the population **mean, variance and standard deviation**
of an array of `N = 5,000,000` pseudo-random floats, using a two-pass algorithm.

---

## 1. The two implementations

Both use the *same* two-pass algorithm so the comparison is fair - the only
difference is *interpreted Python loop* vs *compiled C loop*.

* **Pure Python** - [`stats_pure.py`](stats_pure.py): explicit `for` loops over
  the list (deliberately **not** `numpy`/`statistics`, to isolate the cost of
  the Python interpreter itself).
* **C extension** - [`statsext.c`](statsext.c): uses the CPython C-API
  (`PyArg_ParseTuple`, `PySequence_Fast`, `PyFloat_AsDouble`, `Py_BuildValue`)
  to run the same arithmetic in native C, exposed as `statsext.compute_stats()`.

Build the extension:

```bash
python3 setup.py build_ext --inplace
# -> statsext.cpython-312-aarch64-linux-gnu.so
```

---

## 2. Benchmark results (measured)

`python3 benchmark.py 5000000 5` (best of 5 timed runs each):

```
================================================================
 Statistics benchmark: N = 5,000,000 elements, 5 repeats
================================================================

Pure Python : 0.1940 s
   mean=499.928477 var=83332.450764 stddev=288.673606

C extension : 0.0239 s
   mean=499.928477 var=83332.450764 stddev=288.673606

Max abs difference between results: 0.000e+00

----------------------------------------------------------------
 SPEED-UP (pure Python / C extension): 8.1x faster
----------------------------------------------------------------
```

| Implementation | Time (5M elements, best of 5) | Speed-up |
|---|---|---|
| Pure Python | 0.1940 s | 1.0× (baseline) |
| **C extension** | **0.0239 s** | **≈ 8.1× faster** |

**Correctness:** the maximum absolute difference between the two results is
`0.000e+00` - the C extension returns **bit-identical** values to the pure
Python version, so the speed-up does not cost any accuracy.

---

## 3. Why the C extension is faster

Each iteration of the **pure-Python** loop pays for a lot of interpreter
machinery that has nothing to do with the arithmetic:

1. **Byte-code dispatch** - the CPython evaluation loop decodes and dispatches
   an opcode for every step (`FOR_ITER`, `LOAD_FAST`, `BINARY_OP`, …).
2. **Boxed objects** - every number is a heap-allocated `PyObject` (`float`).
   `total += x` must unbox two `float` objects, add them, and **allocate a new
   `float`** for the result.
3. **Reference counting** - each temporary object's refcount is incremented and
   decremented.

The **C extension** removes all of that from the inner loop: it converts each
element to a native C `double` **once** and then does the additions and
multiplications in raw machine registers, with the compiler free to optimise
(`-O3`). No per-iteration object allocation, no byte-code dispatch.

### Why "only" ~8× and not 100×?

This is an honest and important point. The C loop still calls
`PyFloat_AsDouble()` **once per element** to pull each value out of the Python
list, and that call still touches a Python object. So a meaningful share of the
work - iterating a *Python list of Python floats* - is irreducible while the
data lives as Python objects. The speed-up therefore reflects eliminating the
**interpreter and arithmetic overhead**, not the cost of accessing Python
objects. To go dramatically faster you would store the data in a contiguous C
buffer (e.g. a `numpy` array or the buffer protocol) so the C code never touches
individual `PyObject`s - that is exactly why libraries like NumPy exist. For an
array of *plain Python floats*, ~8× is the expected, realistic gain.

---

## 4. Conclusion

Re-implementing the numeric hot loop as a CPython C extension produced a
**≈ 8× speed-up with identical results**, demonstrating the standard CPython
optimisation pattern: keep orchestration in Python, push the tight numeric loop
into compiled C through the C-API. The bottleneck analysis above also shows the
*limit* of this approach when the data remains a list of Python objects, and the
next step (contiguous numeric buffers) needed to exceed it.

*Raw outputs are in `project3_python_extension/outputs/`
(`benchmark_results.txt`, `build_log.txt`).*
