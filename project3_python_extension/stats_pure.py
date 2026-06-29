"""
stats_pure.py -- the slow, pure-Python statistics routine (Project 3).

This is the baseline the C extension has to beat. It computes the population
mean, variance and standard deviation of a sequence of numbers using explicit
Python ``for`` loops -- deliberately *not* using ``numpy`` or the ``statistics``
module, so that the comparison is "interpreted Python loop" vs. "compiled C
loop" doing exactly the same arithmetic.
"""

import math


def compute_stats(data):
    """Return (mean, variance, stddev) for a sequence of numbers.

    Uses the same two-pass algorithm as the C extension so the two
    implementations are directly comparable.
    """
    n = len(data)
    if n == 0:
        raise ValueError("compute_stats() arg is an empty sequence")

    # Pass 1: mean
    total = 0.0
    for x in data:
        total += x
    mean = total / n

    # Pass 2: variance
    sq_accum = 0.0
    for x in data:
        d = x - mean
        sq_accum += d * d
    variance = sq_accum / n

    return mean, variance, math.sqrt(variance)


if __name__ == "__main__":
    sample = [float(i) for i in range(10)]
    print("data   :", sample)
    print("stats  :", compute_stats(sample))
