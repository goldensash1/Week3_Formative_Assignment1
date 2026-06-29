/*
 * statsext.c  --  A CPython C extension for fast array statistics.
 *
 * Project 3 asks us to speed up a slow pure-Python numeric routine by
 * re-implementing the hot loop as a C extension that uses the CPython C-API.
 *
 * The module exports one function:
 *
 *     statsext.compute_stats(seq) -> (mean, variance, stddev)
 *
 * It accepts any Python sequence of numbers (a list or tuple), pulls each
 * element down to a native C double with PyFloat_AsDouble(), and runs a
 * two-pass mean/variance computation entirely in C. Because the per-element
 * work happens in compiled C instead of the CPython byte-code interpreter,
 * it avoids the boxing/unboxing and dispatch overhead that makes the pure
 * Python loop slow.
 *
 * Build:  python3 setup.py build_ext --inplace
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

/*
 * compute_stats(seq) -> (mean, variance, stddev)
 *
 * Two-pass population statistics:
 *   pass 1: mean   = (1/n) * sum(x_i)
 *   pass 2: var    = (1/n) * sum((x_i - mean)^2)
 *           stddev = sqrt(var)
 */
static PyObject *statsext_compute_stats(PyObject *self, PyObject *args)
{
    PyObject *seq_arg = NULL;

    if (!PyArg_ParseTuple(args, "O", &seq_arg))
        return NULL;

    /* PySequence_Fast gives O(1) indexed access to a list or tuple and
     * raises TypeError for anything that is not a sequence. */
    PyObject *seq = PySequence_Fast(seq_arg,
                                    "compute_stats() requires a sequence");
    if (seq == NULL)
        return NULL;

    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    if (n == 0) {
        Py_DECREF(seq);
        PyErr_SetString(PyExc_ValueError,
                        "compute_stats() arg is an empty sequence");
        return NULL;
    }

    /* ---- Pass 1: sum and mean ---- */
    double sum = 0.0;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(seq, i); /* borrowed ref */
        double x = PyFloat_AsDouble(item);
        if (x == -1.0 && PyErr_Occurred()) {
            Py_DECREF(seq);
            return NULL;                 /* non-numeric element */
        }
        sum += x;
    }
    double mean = sum / (double)n;

    /* ---- Pass 2: variance ---- */
    double sq_accum = 0.0;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
        double x = PyFloat_AsDouble(item);
        if (x == -1.0 && PyErr_Occurred()) {
            Py_DECREF(seq);
            return NULL;
        }
        double d = x - mean;
        sq_accum += d * d;
    }
    double variance = sq_accum / (double)n;
    double stddev = sqrt(variance);

    Py_DECREF(seq);

    /* Build the (mean, variance, stddev) result tuple. */
    return Py_BuildValue("(ddd)", mean, variance, stddev);
}

/* ---- Module method table ---- */
static PyMethodDef StatsExtMethods[] = {
    {"compute_stats", statsext_compute_stats, METH_VARARGS,
     "compute_stats(seq) -> (mean, variance, stddev)\n\n"
     "Compute population mean, variance and standard deviation of a\n"
     "sequence of numbers using a two-pass algorithm implemented in C."},
    {NULL, NULL, 0, NULL}        /* sentinel */
};

/* ---- Module definition ---- */
static struct PyModuleDef statsextmodule = {
    PyModuleDef_HEAD_INIT,
    "statsext",                                  /* m_name */
    "Fast array statistics implemented in C.",   /* m_doc  */
    -1,                                          /* m_size */
    StatsExtMethods
};

/* ---- Module init ---- */
PyMODINIT_FUNC PyInit_statsext(void)
{
    return PyModule_Create(&statsextmodule);
}
