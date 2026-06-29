"""
setup.py -- build script for the statsext C extension (Project 3).

Build the extension in-place (next to this file) with:

    python3 setup.py build_ext --inplace

That produces a shared object such as
``statsext.cpython-312-x86_64-linux-gnu.so`` which can then be imported as a
normal module:  ``import statsext``.
"""

from setuptools import setup, Extension

statsext_module = Extension(
    "statsext",
    sources=["statsext.c"],
    extra_compile_args=["-O3"],   # optimise the C hot loop
)

setup(
    name="statsext",
    version="1.0",
    description="Fast array statistics implemented as a CPython C extension",
    ext_modules=[statsext_module],
)
