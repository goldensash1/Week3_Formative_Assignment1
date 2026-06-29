# Week 3 — Formative Assignment 1 (Linux Programming)

Four small systems-programming projects exploring how Linux programs are
structured, how they talk to the kernel, how to accelerate them, and how they
react to signals. **Every command output committed here is real** — regenerated
inside a pinned Ubuntu 24.04 Linux toolchain (see *Reproducing* below).

| # | Project | Tools / APIs | Key result |
|---|---------|--------------|------------|
| 1 | Investigating a Suspicious Binary | `gcc`, `file`, `readelf`, `objdump`, `nm` | Identified `data_sync` as a benign file-sync tool by ELF imports alone |
| 2 | System Call Monitoring Tool | `strace` | Traced & classified every file/process syscall of a backup program |
| 3 | Python Performance Extension | CPython C-API, `setuptools` | **≈ 8× speed-up**, bit-identical results |
| 4 | Signal-Based Server Controller | `sigaction(2)`, `kill(1)` | Handles SIGINT / SIGUSR1 / SIGTERM correctly |

---

## Repository structure

```
Week3_Formative_Assignment1/
├── README.md                     <- this file
├── Dockerfile                    <- pinned Linux toolchain (reproducibility)
├── run_all.sh                    <- regenerate every output with one command
├── scripts/                      <- one driver script per project
│   ├── p1_analyze.sh  p2_trace.sh  p3_benchmark.sh  p4_signals.sh
│
├── project1_suspicious_binary/
│   ├── data_sync.c               <- source of the "suspicious" binary
│   ├── forensic_report.md        <- 1–2 page forensic analysis
│   └── outputs/                  <- file/readelf/objdump/nm outputs
│
├── project2_syscall_monitor/
│   ├── backup_tool.c             <- file create / write / read program
│   ├── syscall_report.md         <- syscall summary table + interpretation
│   └── outputs/                  <- strace_output.txt, strace_summary.txt, …
│
├── project3_python_extension/
│   ├── stats_pure.py             <- pure-Python baseline
│   ├── statsext.c                <- CPython C extension
│   ├── setup.py  benchmark.py
│   ├── performance_report.md     <- benchmark + analysis
│   └── outputs/                  <- benchmark_results.txt, build_log.txt
│
└── project4_signal_service/
    ├── monitor_service.c         <- signal-driven service
    ├── signal_report.md          <- explanation + demonstration logs
    └── outputs/                  <- demo_sigint.txt, demo_sigterm.txt
```

---

## Reproducing every output

All tools are Linux-specific (ELF, `strace`, the CPython C-API). A `Dockerfile`
pins them so the results reproduce on any host, including Apple-Silicon macOS.

```bash
# from this directory
docker build -t week3-linux .
docker run --rm -v "$PWD":/work -w /work week3-linux ./run_all.sh
```

Or run a single project:

```bash
docker run --rm -v "$PWD":/work -w /work week3-linux bash scripts/p1_analyze.sh
docker run --rm -v "$PWD":/work -w /work week3-linux bash scripts/p2_trace.sh
docker run --rm -v "$PWD":/work -w /work week3-linux bash scripts/p3_benchmark.sh
docker run --rm -v "$PWD":/work -w /work week3-linux bash scripts/p4_signals.sh
```

On a native Linux machine you can skip Docker and run the scripts directly
(needs `build-essential binutils strace file python3 python3-dev`).

**Toolchain used for the committed outputs:** Ubuntu 24.04, GCC 13.3.0,
GNU binutils 2.42, strace 6.8, Python 3.12.3, architecture `aarch64`
(64-bit ARM Linux — the analysis techniques are identical on x86-64).

---

## Project 1 — Investigating a Suspicious Binary

`data_sync.c` is compiled into an ELF executable that is then analysed
**without running it**. Static inspection answers:

* **Entry point:** `0xcc0` (PIE offset) → glibc `_start` → `__libc_start_main`
  → `main`. (`readelf -h`, `objdump -f`, `objdump -d`)
* **Imported functions:** `opendir/readdir/closedir`, `stat`, `mkdir`,
  `fopen/fread/fwrite/fclose`, `malloc/free`, `strcmp`, `printf`… all from
  `libc.so.6`. (`nm -D`, `objdump -T`, `readelf -d`)
* **Sections:** `.text`, `.rodata`, `.data`, `.bss`, `.plt/.got`,
  `.dynsym/.dynstr`, … (`objdump -h`, `readelf -S`)

**Conclusion:** directory traversal + per-file `stat` + buffered copy =
a one-way **file-synchronisation / backup tool**, with *no* network, exec,
shell or crypto imports → benign. Full write-up:
[`project1_suspicious_binary/forensic_report.md`](project1_suspicious_binary/forensic_report.md).

---

## Project 2 — System Call Monitoring Tool

`backup_tool.c` creates a backup file, writes logs, and reads the file back,
then is traced with `strace`. The report classifies every syscall into
**file-related** (`openat`, `read`, `write`, `close`, `lseek`, `fstat`,
`newfstatat`) and **process/memory-related** (`execve`, `getpid`, `brk`,
`mmap`, `mprotect`, …), distinguishing application logic from runtime start-up.
Full write-up + table:
[`project2_syscall_monitor/syscall_report.md`](project2_syscall_monitor/syscall_report.md).

---

## Project 3 — Building a Python Performance Extension

A pure-Python mean/variance/stddev routine is re-implemented as a CPython C
extension (`statsext`). On 5,000,000 floats the C version is **≈ 8.1× faster**
(0.1940 s → 0.0239 s) with a maximum result difference of `0.000e+00`. The
report explains *why* (no byte-code dispatch, no boxed-object allocation, no
refcounting in the inner loop) and the realistic limit of the approach. Full
write-up:
[`project3_python_extension/performance_report.md`](project3_python_extension/performance_report.md).

---

## Project 4 — Signal-Based Server Controller

`monitor_service.c` runs continuously, prints a heartbeat every 5 s, and handles
three signals via `sigaction(2)` using async-signal-safe handlers:

| Signal | Message | Behaviour |
|---|---|---|
| SIGINT | `Monitor Service shutting down safely.` | graceful exit (0) |
| SIGUSR1 | `System status requested by administrator.` | report status, keep running |
| SIGTERM | `Emergency shutdown signal received.` | emergency exit (2) |

Two captured demonstration runs prove each path. Full write-up:
[`project4_signal_service/signal_report.md`](project4_signal_service/signal_report.md).

---

*Author: S. Munyankindi · ALU — Linux Programming · Week 3 Formative 1*
