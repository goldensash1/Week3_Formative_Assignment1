# Project 1 - Forensic Report: Investigating the Suspicious Binary `data_sync`

**Analyst:** S. Munyankindi
**Method:** Static analysis only - the binary was **never executed**. All
conclusions are drawn from the ELF structure using `file`, `readelf`,
`objdump`, and `nm`.

---

## 1. Summary

A system administrator found an unknown executable named **`data_sync`** on a
server. Working only from the on-disk ELF image, the binary was identified as a
**benign one-way file-synchronisation / backup utility**: it walks a source
directory, inspects each entry's metadata, and copies regular files into a
destination directory. There is **no evidence of networking, process injection,
encryption, or persistence** - the imported symbols are confined to ordinary
file-system and standard-I/O routines from the C library.

---

## 2. File type and overall identity

```
$ file data_sync
data_sync: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV),
dynamically linked, interpreter /lib/ld-linux-aarch64.so.1,
BuildID[sha1]=f157..., for GNU/Linux 3.7.0, with debug_info, not stripped
```

* **ELF 64-bit, little-endian, AArch64** - a 64-bit ARM Linux executable.
* **PIE (Position-Independent Executable)** - confirmed below by ELF type `DYN`.
* **Dynamically linked** against the system C library; the program loader is
  `ld-linux-aarch64.so.1`.
* **Not stripped, with `debug_info`** - symbol and debug data are present, which
  greatly aids analysis (a deliberately malicious binary is usually stripped).

---

## 3. Entry point

`readelf -h` and `objdump -f` agree on the entry point:

```
$ readelf -h data_sync
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           AArch64
  Entry point address:               0xcc0

$ objdump -f data_sync
  start address 0x0000000000000cc0
```

Because the file is a PIE, `0xcc0` is a **file-relative offset**; the loader
adds a random base at run time (ASLR). Disassembly shows that this offset is the
start of the standard C runtime stub `_start`, which lives in the `.text`
section and immediately hands control to `__libc_start_main`, which in turn
calls `main`:

```
$ objdump -d data_sync
0000000000000cc0 <_start>:
     cc0:   d503201f    nop
     ...
     cec:   97ffff89    bl   b10 <__libc_start_main@plt>
```

**Interpretation:** the entry point is a normal glibc start-up sequence - there
is no custom or obfuscated bootstrap, which is consistent with an
ordinary program compiled by GCC.

---

## 4. Imported library functions (the strongest behavioural evidence)

`nm -D` lists the dynamic symbol table; the symbols marked **`U`** (undefined)
are resolved from a shared library at run time - i.e. they are the functions the
binary **imports**. The `readelf -d` output confirms the only `NEEDED` library
is **`libc.so.6`** (plus the dynamic loader), so every import comes from the
standard C library.

```
$ nm -D data_sync | grep ' U '
   U opendir      U readdir      U closedir     <- directory traversal
   U stat         U mkdir                       <- file metadata + dir creation
   U fopen        U fread        U fwrite   U fclose   <- buffered file copy
   U malloc       U free                        <- dynamic copy buffer
   U strcmp       U strerror     U snprintf     <- string / path / error handling
   U printf       U fprintf      U stderr       <- console reporting
   U time         U difftime                    <- run timing
   U __libc_start_main  U __stack_chk_fail  U __stack_chk_guard
   U __errno_location   U abort                 <- glibc runtime / stack-guard
```

Grouping these by purpose reveals the program's logic without running it:

| Imported functions | What they imply the program does |
|---|---|
| `opendir`, `readdir`, `closedir` | Enumerates the contents of a directory |
| `stat`, `mkdir` | Reads file metadata (size/mtime/type) and creates a directory |
| `fopen`, `fread`, `fwrite`, `fclose` | Opens files and copies their bytes |
| `malloc`, `free` | Allocates/frees a heap buffer for copying |
| `strcmp` | Filters entries (e.g. skips `.` and `..`) |
| `printf`, `fprintf`, `snprintf`, `strerror`, `stderr` | Prints progress and error messages |
| `time`, `difftime` | Measures how long the run took |
| `__stack_chk_fail`, `__stack_chk_guard` | Stack-smashing protection (`-fstack-protector`) - a safety feature, not malware |

**Conclusion from imports:** *directory enumeration* + *per-file `stat`* +
*buffered file copying* is the canonical fingerprint of a **file copy /
synchronisation / backup tool**. The presence of `stat`/`difftime` suggests it
compares timestamps to decide what to copy (incremental sync), and `mkdir`
suggests it ensures the destination directory exists.

**Equally important - what is *absent*:** there are **no** imports for
networking (`socket`, `connect`, `send`, `recv`), no process control
(`fork`, `execve`, `ptrace`), no shells (`system`, `popen`), and no crypto
(`crypt`, `EVP_*`). Their absence is strong evidence the binary is **not** a
backdoor, downloader, or ransomware.

---

## 5. ELF sections

```
$ objdump -h data_sync   (also cross-checked with readelf -S)
```

| Section | Purpose / what it tells us |
|---|---|
| `.interp` | Path to the dynamic loader (`ld-linux-aarch64.so.1`) → dynamically linked |
| `.text` (VMA `0xcc0`, code) | The executable machine code - entry point lives here |
| `.rodata` | Read-only constants: format strings such as the usage banner |
| `.data` | Initialised writable globals |
| `.bss` | Zero-initialised globals (occupies no file space) |
| `.plt` / `.got` | Procedure Linkage / Global Offset Tables - the lazy-binding stubs used to call the imported libc functions |
| `.dynsym` / `.dynstr` | Dynamic symbol table & strings (the import list) |
| `.rela.dyn` / `.rela.plt` | Relocations applied at load time |
| `.init` / `.fini` / `.init_array` | Constructor/destructor hooks run before/after `main` |
| `.note.gnu.build-id` | A SHA-1 build ID - useful for tracking the exact build |
| `.symtab` + `.debug_*` | Full symbols and DWARF debug info (binary is *not stripped*) |

The section layout is completely standard for a GCC-compiled, dynamically
linked PIE. Nothing unusual was found - for example, there are no oversized or
oddly named sections that might hide a packed/encrypted payload, and code lives
only in `.text`/`.plt`/`.init`/`.fini` as expected.

---

## 6. Overall forensic assessment

| Question | Finding |
|---|---|
| What kind of file is it? | A 64-bit ARM Linux, dynamically linked, **PIE** executable, not stripped |
| Where does execution begin? | Entry `0xcc0` → glibc `_start` → `__libc_start_main` → `main` (standard) |
| What does it depend on? | Only `libc.so.6` (and the dynamic loader) |
| What does it likely *do*? | Walks a source directory (`opendir`/`readdir`), `stat`s each file, and copies regular files into a destination directory (`fopen`/`fread`/`fwrite`), creating that directory if needed (`mkdir`) - a **one-way file synchronisation / backup tool** |
| Is it malicious? | **No indicators of compromise.** No network, exec, shell, ptrace, or crypto imports; stack protection is enabled; symbols/debug info are intact |
| Confidence | **High** - the import set is unambiguous and the absence of dangerous calls is verifiable directly from `nm -D` |

**Recommendation:** `data_sync` is consistent with a legitimate administrative
file-sync utility. As a precaution before clearing it, the administrator should
(a) record the build-id hash and compare it against known-good binaries, and
(b) confirm provenance (package owner / who installed it). The static evidence,
however, shows no malicious capability.

*All command outputs cited above are stored verbatim in
`project1_suspicious_binary/outputs/`.*
