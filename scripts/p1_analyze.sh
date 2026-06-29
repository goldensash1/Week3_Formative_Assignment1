#!/usr/bin/env bash
# Project 1 -- compile data_sync and capture all ELF static-analysis outputs.
# Run inside the week3-linux container from the repo root.
set -u
cd "$(dirname "$0")/../project1_suspicious_binary"
OUT=outputs
mkdir -p "$OUT"

echo "[p1] compiling data_sync (dynamically linked, with symbols)..."
gcc -O0 -g -o data_sync data_sync.c

echo "[p1] file(1) classification ..............................."
file data_sync | tee "$OUT/01_file.txt"

echo "[p1] ELF header / entry point (readelf -h) ..............."
readelf -h data_sync > "$OUT/02_readelf_header.txt"

echo "[p1] entry point via objdump -f ..........................."
objdump -f data_sync > "$OUT/03_objdump_fileheader.txt"

echo "[p1] section headers (objdump -h) ........................."
objdump -h data_sync > "$OUT/04_objdump_sections.txt"

echo "[p1] section headers (readelf -S) ........................."
readelf -S data_sync > "$OUT/05_readelf_sections.txt"

echo "[p1] full symbol table (nm) ..............................."
nm data_sync > "$OUT/06_nm_symbols.txt"

echo "[p1] imported/undefined dynamic symbols (nm -D) ..........."
nm -D data_sync > "$OUT/07_nm_dynamic.txt"
# 'U' = undefined => resolved from a shared library at run time = an import.
nm -D data_sync | grep ' U ' > "$OUT/08_nm_imports_only.txt"

echo "[p1] dynamic symbols (objdump -T) ........................."
objdump -T data_sync > "$OUT/09_objdump_dynsym.txt"

echo "[p1] shared library dependencies (readelf -d) ............."
readelf -d data_sync > "$OUT/10_readelf_dynamic.txt"

echo "[p1] disassembly of the entry/start code (objdump -d) ....."
objdump -d data_sync > "$OUT/11_objdump_disasm.txt"

echo "[p1] DONE. Outputs in project1_suspicious_binary/$OUT/"
