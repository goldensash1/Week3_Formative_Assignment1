# Project 2 - System Call Monitoring Report (`backup_tool`)

**Goal:** understand how a small file-backup program interacts with the
operating system by tracing its system calls with **`strace`**.

**Program under test:** `backup_tool.c` - it (1) **creates** a backup data
file, (2) **writes** payload bytes into it, (3) **appends log lines** to a log
file, and (4) **reads** the data file back and prints it. It uses the low-level
POSIX wrappers (`open`/`write`/`read`/`close`) so that each operation maps
almost one-to-one onto a kernel system call.

---

## 1. How the program was traced

```bash
gcc -O0 -g -o backup_tool backup_tool.c

# Full, timestamped trace (-tt = µs timestamps, -T = time-in-call, -f = follow forks)
strace -f -tt -T -o outputs/strace_output.txt ./backup_tool

# Aggregated summary table
strace -f -c   -o outputs/strace_summary.txt ./backup_tool

# File-only view
strace -f -e trace=file,desc -tt -o outputs/strace_file_calls.txt ./backup_tool
```

Program output confirming it worked:

```
----- recovered backup contents -----
BACKUP RECORD
=============
server: web-01
status: OK
files : 3
[backup_tool] wrote 64 bytes, read back 64 bytes
```

---

## 2. The program's own file operations (extracted from the trace)

These are the lines from `strace_output.txt` that correspond directly to the
program's logic (process PID 18 in this run):

```
execve("./backup_tool", ["./backup_tool"], 0x... /* 8 vars */) = 0
openat(AT_FDCWD, "backup_data.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3   # create
write(3, "BACKUP RECORD\n=============\nserv"..., 64) = 64                # write data
openat(AT_FDCWD, "backup.log", O_WRONLY|O_CREAT|O_APPEND, 0644) = 3      # open log
getpid()                                = 18                              # stamp log line
write(3, "[2026-... (pid 18) created...\n", ...) = ...                    # write log
openat(AT_FDCWD, "backup_data.txt", O_RDONLY) = 3                        # reopen to read
write(1, "----- recovered backup contents "..., 38) = 38                 # stdout banner
read(3, "BACKUP RECORD\n=============\nserv"..., 256) = 64               # read back
write(1, "BACKUP RECORD\n=============\nserv"..., 64) = 64               # echo to stdout
write(1, "\n[backup_tool] wrote 64 bytes, r"..., 50) = 50               # report
```

Every step the C program performs is visible: the file is **created**
(`openat` with `O_CREAT|O_TRUNC`), **written** (`write` to fd 3), the log file
is **opened for append** and written, and the data file is **reopened read-only**
and **read** back (`read` returns 64 - exactly the bytes written).

---

## 3. `strace -c` summary (measured)

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 21.43    0.000228          28         8           openat
 20.30    0.000216         216         1           execve
 10.34    0.000110          18         6           mmap
  9.96    0.000106          13         8           close
  7.61    0.000081          11         7           write
  7.24    0.000077          19         4           mprotect
  4.14    0.000044           8         5           read
  3.95    0.000042          14         3           munmap
  2.82    0.000030           7         4           fstat
  2.63    0.000028           9         3           brk
  2.07    0.000022          11         2           newfstatat
  1.79    0.000019           6         3           getpid
  1.03    0.000011          11         1         1 faccessat
  1.03    0.000011          11         1           set_robust_list
  1.03    0.000011          11         1           rseq
  0.85    0.000009           9         1           getrandom
  0.66    0.000007           7         1           prlimit64
  0.56    0.000006           6         1           lseek
  0.56    0.000006           6         1           set_tid_address
------ ----------- ----------- --------- --------- ----------------
100.00    0.001064          17        61         1 total
```

> Note: counts include start-up calls the C runtime/dynamic loader make before
> `main` runs (e.g. `mmap`/`mprotect` to map `libc.so.6`). Those are part of how
> *every* dynamically linked program interacts with the OS, so they are included
> and labelled below.

---

## 4. System-call summary table (interpretation)

### 4.1 File-related system calls

| System call | Count | Category | Purpose in this program |
|---|---|---|---|
| `execve` | 1 | file (program load) | Loads and starts the `backup_tool` executable |
| `openat` | 8 | file | Opens/creates files by path relative to `AT_FDCWD`. Used for `backup_data.txt` (create + reopen) and `backup.log`; the rest open `libc.so.6`/loader cache during start-up |
| `read` | 5 | file/desc | Reads bytes from a file descriptor - reads `backup_data.txt` back (returns 64) and reads ELF headers at start-up |
| `write` | 7 | file/desc | Writes bytes to a descriptor - payload to `backup_data.txt`, lines to `backup.log`, and output to stdout (fd 1) |
| `close` | 8 | file/desc | Releases file descriptors after each open |
| `lseek` | 1 | file/desc | Repositions the file offset (used internally by the C library) |
| `fstat` / `newfstatat` | 4 / 2 | file (metadata) | Queries file metadata (size, type) - e.g. to size I/O buffers |
| `faccessat` | 1 | file | Checks file accessibility during loader/library resolution (1 expected `ENOENT`) |
| `getrandom` | 1 | file/security | Supplies entropy used to initialise the stack-protector canary |

### 4.2 Process- / memory-related system calls

| System call | Count | Category | Purpose |
|---|---|---|---|
| `execve` | 1 | process | Replaces the shell's image with `backup_tool` (start of the process) |
| `getpid` | 3 | process | Returns the PID - the program stamps each log line with it |
| `brk` | 3 | process/memory | Grows the heap (used by the C library's allocator) |
| `mmap` | 6 | process/memory | Maps memory - loads `libc.so.6` and allocates anonymous memory |
| `mprotect` | 4 | process/memory | Changes page protections (e.g. makes the GOT read-only - RELRO hardening) |
| `munmap` | 3 | process/memory | Unmaps memory regions used during start-up |
| `set_tid_address`, `set_robust_list`, `rseq` | 1 each | process/threading | Kernel/thread bookkeeping set up by glibc at start-up |
| `prlimit64` | 1 | process | Queries/sets resource limits (e.g. stack size) during start-up |

### 4.3 What the trace teaches us

* The program's behaviour is **fully explained by file I/O**: open → write →
  close to create the backup, open(append) → write to log, open(read) → read to
  verify. The byte counts (`write ... = 64`, `read ... = 64`) prove the data
  round-trips correctly.
* `getpid` being called three times matches the three `write_log()` calls (each
  log line embeds the PID).
* The memory/process syscalls (`mmap`, `mprotect`, `brk`, `set_robust_list`,
  …) are **start-up overhead** common to every dynamically linked Linux
  program - they show *how* the OS loads a process and its shared libraries,
  not application logic. Recognising the difference between application syscalls
  and runtime/start-up syscalls is the key skill `strace` teaches.

*Full traces are stored in `project2_syscall_monitor/outputs/`
(`strace_output.txt`, `strace_summary.txt`, `strace_file_calls.txt`).*
