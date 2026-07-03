# Project 4 - Signal-Based Server Controller (`monitor_service`)

**Goal:** a long-running background service that prints a heartbeat every 5
seconds and responds to administrative commands delivered as Unix signals.

---

## 1. Compilation and execution

```bash
gcc -O0 -g -o monitor_service monitor_service.c
./monitor_service          # prints its own PID on start-up
```

Signals are sent from another terminal with `kill`:

```bash
kill -SIGUSR1 <pid>   # request a status report
kill -SIGINT  <pid>   # (or Ctrl-C) graceful shutdown
kill -SIGTERM <pid>   # emergency shutdown
```

---

## 2. How the program registers signal handlers

The service installs handlers with **`sigaction(2)`** (preferred over
`signal(2)` because its behaviour is portable and well-defined). A small helper
registers each one:

```c
static int install_handler(int signo, void (*fn)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fn;
    sigemptyset(&sa.sa_mask);   // no extra signals blocked during the handler
    sa.sa_flags = 0;            // no SA_RESTART -> sleep() returns on a signal
    return sigaction(signo, &sa, NULL);
}
...
install_handler(SIGINT,  handle_sigint);
install_handler(SIGUSR1, handle_sigusr1);
install_handler(SIGTERM, handle_sigterm);
```

**Safe handler design.** Signal handlers run asynchronously, so they only do
work that is *async-signal-safe*. Each handler:

* prints its fixed message with **`write(2)`** (async-signal-safe), **not**
  `printf` (which is not safe inside a handler), and
* sets a **`volatile sig_atomic_t`** flag (`g_shutdown`, `g_status`,
  `g_terminate`) that the main loop reads.

The `main()` loop runs `sleep(5)` for the heartbeat; when a signal arrives,
`sleep` returns early, the loop inspects the flags, and reacts. This is the
standard, race-free "set a flag in the handler, act in the main loop" pattern.

---

## 3. What happens for each signal

| Signal | Handler message (via `write`) | Effect on the service |
|---|---|---|
| **SIGINT** (Ctrl-C / `kill -SIGINT`) | `Monitor Service shutting down safely.` | Sets `g_shutdown`; loop exits, runs cleanup, returns exit code **0** (graceful) |
| **SIGUSR1** (`kill -SIGUSR1`) | `System status requested by administrator.` | Sets `g_status`; loop prints a status line and **keeps running** |
| **SIGTERM** (`kill -SIGTERM`) | `Emergency shutdown signal received.` | Sets `g_terminate`; loop performs an immediate emergency exit with code **2** |

---

## 4. Demonstration (captured terminal logs)

### Demonstration 1 - two SIGUSR1 status requests, then SIGINT (graceful)

Commands sent: `kill -SIGUSR1`, `kill -SIGUSR1`, `kill -SIGINT`.
Captured in `outputs/demo_sigint.txt`:

```
[Monitor Service] Started. PID = 14
[Monitor Service] Send signals with: kill -SIGUSR1 14
[Monitor Service] System running normally...

[Monitor Service] System status requested by administrator.
[Monitor Service] STATUS: uptime OK, all checks passed.
[Monitor Service] System running normally...

[Monitor Service] System status requested by administrator.
[Monitor Service] STATUS: uptime OK, all checks passed.
[Monitor Service] System running normally...

[Monitor Service] Monitor Service shutting down safely.
[Monitor Service] Cleanup complete. Goodbye.
```
→ The service kept running after each SIGUSR1 and only stopped on SIGINT,
exiting cleanly (**exit code 0**).

### Demonstration 2 - SIGUSR1, then SIGTERM (emergency)

Commands sent: `kill -SIGUSR1`, `kill -SIGTERM`.
Captured in `outputs/demo_sigterm.txt`:

```
[Monitor Service] Started. PID = 18
[Monitor Service] Send signals with: kill -SIGUSR1 18
[Monitor Service] System running normally...

[Monitor Service] System status requested by administrator.
[Monitor Service] STATUS: uptime OK, all checks passed.

[Monitor Service] Emergency shutdown signal received.
[Monitor Service] Performing emergency shutdown now.
```
→ SIGTERM triggered an immediate emergency shutdown (**exit code 2**).

---

## 5. Conclusion

The service demonstrates the full life-cycle of OS signal handling: registering
handlers with `sigaction`, doing only async-signal-safe work inside them,
communicating with the main loop through `volatile sig_atomic_t` flags, and
distinguishing three administrative commands - **status (SIGUSR1)**, **graceful
shutdown (SIGINT)**, and **emergency shutdown (SIGTERM)** - each producing the
exact required message and the correct continue-or-exit behaviour.

*Raw demonstration logs are in `project4_signal_service/outputs/`.*
