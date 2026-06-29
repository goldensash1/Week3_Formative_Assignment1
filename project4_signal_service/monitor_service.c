/*
 * monitor_service.c  --  A signal-driven "server monitoring" service.
 *
 * Project 4: a long-running background-style service that prints a heartbeat
 * every 5 seconds and reacts to administrative commands delivered as Unix
 * signals.
 *
 *   Signal     Behaviour
 *   --------   -----------------------------------------------------------
 *   SIGINT     Graceful shutdown: "Monitor Service shutting down safely."
 *   SIGUSR1    Status report:    "System status requested by administrator."
 *   SIGTERM    Emergency stop:   "Emergency shutdown signal received." + exit
 *
 * Signal handlers are installed with sigaction(2) (more portable and better
 * defined than signal(2)). The handlers themselves only do async-signal-safe
 * work: they either write() a fixed string directly to stdout, or set a
 * volatile sig_atomic_t flag that the main loop checks. This is the standard,
 * safe pattern -- it never calls printf()/exit() from inside a handler.
 *
 * Build:  gcc -O0 -g -o monitor_service monitor_service.c
 * Run:    ./monitor_service
 * Signal: kill -SIGUSR1 <pid> ; kill -SIGINT <pid> ; kill -SIGTERM <pid>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/*
 * Flags set by the signal handlers and consumed by the main loop.
 * 'volatile sig_atomic_t' is the only type guaranteed safe to share between
 * a handler and normal code.
 */
static volatile sig_atomic_t g_shutdown = 0;   /* set by SIGINT  */
static volatile sig_atomic_t g_terminate = 0;  /* set by SIGTERM */
static volatile sig_atomic_t g_status = 0;     /* set by SIGUSR1 */

/* write(2) is async-signal-safe; printf(3) is not, so handlers use write(). */
static void safe_print(const char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

/* SIGINT -> request a graceful shutdown. */
static void handle_sigint(int signum)
{
    (void)signum;
    safe_print("\n[Monitor Service] Monitor Service shutting down safely.\n");
    g_shutdown = 1;
}

/* SIGUSR1 -> administrator requested a status report. */
static void handle_sigusr1(int signum)
{
    (void)signum;
    safe_print("\n[Monitor Service] System status requested by administrator.\n");
    g_status = 1;
}

/* SIGTERM -> emergency shutdown. */
static void handle_sigterm(int signum)
{
    (void)signum;
    safe_print("\n[Monitor Service] Emergency shutdown signal received.\n");
    g_terminate = 1;
}

/* Install one handler with sigaction(2). Returns 0 on success, -1 on error. */
static int install_handler(int signo, void (*fn)(int))
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fn;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;            /* no SA_RESTART: let sleep() return on signal */
    return sigaction(signo, &sa, NULL);
}

int main(void)
{
    if (install_handler(SIGINT,  handle_sigint)  != 0 ||
        install_handler(SIGUSR1, handle_sigusr1) != 0 ||
        install_handler(SIGTERM, handle_sigterm) != 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("[Monitor Service] Started. PID = %d\n", getpid());
    printf("[Monitor Service] Send signals with: kill -SIGUSR1 %d\n",
           getpid());
    fflush(stdout);

    /* Main service loop: heartbeat every 5 seconds, react to flags. */
    while (!g_shutdown && !g_terminate) {
        /* sleep() returns early (with remaining time) if a signal arrives. */
        unsigned int remaining = sleep(5);

        if (g_terminate) {
            printf("[Monitor Service] Performing emergency shutdown now.\n");
            fflush(stdout);
            return 2;          /* distinct exit code for emergency stop */
        }

        if (g_shutdown)
            break;             /* leave loop, do graceful cleanup below */

        if (g_status) {
            printf("[Monitor Service] STATUS: uptime OK, all checks passed.\n");
            fflush(stdout);
            g_status = 0;      /* acknowledge and reset */
        }

        /* Only emit the heartbeat if a full interval elapsed (not on EINTR). */
        if (remaining == 0) {
            printf("[Monitor Service] System running normally...\n");
            fflush(stdout);
        }
    }

    printf("[Monitor Service] Cleanup complete. Goodbye.\n");
    fflush(stdout);
    return EXIT_SUCCESS;
}
