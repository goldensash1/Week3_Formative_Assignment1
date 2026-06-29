/*
 * backup_tool.c  --  A miniature file-backup program for syscall tracing.
 *
 * This is the program traced in Project 2. It exercises the three file
 * operations the assignment asks for -- creation, writing logs, and reading --
 * using the low-level POSIX I/O primitives (open / write / read / close)
 * rather than the buffered <stdio.h> wrappers. Going through the raw wrappers
 * means each operation maps almost one-to-one onto a kernel system call, so
 * the strace output is clean and easy to interpret.
 *
 * What it does, in order:
 *   1. Creates a backup data file               -> openat(O_CREAT|O_WRONLY)
 *   2. Writes payload bytes into it             -> write()
 *   3. Appends a line to a log file             -> openat(O_APPEND), write()
 *   4. Reads the backup file back and prints it -> openat(O_RDONLY), read()
 *   5. Reports the run on stdout                -> write() to fd 1
 *
 * Build:  gcc -O0 -g -o backup_tool backup_tool.c
 * Trace:  strace -f -tt -o strace_output.txt ./backup_tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      /* open, O_* flags */
#include <unistd.h>     /* read, write, close, getpid */
#include <sys/stat.h>   /* mode constants */
#include <sys/types.h>
#include <time.h>

#define DATA_FILE   "backup_data.txt"
#define LOG_FILE    "backup.log"
#define READ_BUF    256

/* Append one timestamped line to the log file (file creation + write). */
static int write_log(const char *message)
{
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open log");
        return -1;
    }

    char line[320];
    time_t now = time(NULL);
    char stamp[64];
    struct tm *tm_info = localtime(&now);
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", tm_info);

    int len = snprintf(line, sizeof(line), "[%s] (pid %d) %s\n",
                       stamp, getpid(), message);

    if (write(fd, line, (size_t)len) != len) {
        perror("write log");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int main(void)
{
    const char *payload =
        "BACKUP RECORD\n"
        "=============\n"
        "server: web-01\n"
        "status: OK\n"
        "files : 3\n";

    /* --- Step 1 & 2: create the backup data file and write to it. --- */
    int out_fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        perror("open data (create)");
        return EXIT_FAILURE;
    }

    ssize_t written = write(out_fd, payload, strlen(payload));
    if (written < 0) {
        perror("write data");
        close(out_fd);
        return EXIT_FAILURE;
    }
    close(out_fd);
    write_log("created backup data file");

    /* --- Step 3: read the backup file back. --- */
    int in_fd = open(DATA_FILE, O_RDONLY);
    if (in_fd < 0) {
        perror("open data (read)");
        return EXIT_FAILURE;
    }

    char buf[READ_BUF];
    ssize_t nread;
    ssize_t total_read = 0;

    /* Echo the recovered content to stdout via the write() syscall. */
    const char *banner = "----- recovered backup contents -----\n";
    write(STDOUT_FILENO, banner, strlen(banner));

    while ((nread = read(in_fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, (size_t)nread);
        total_read += nread;
    }
    close(in_fd);
    write_log("verified backup by reading it back");

    /* --- Step 5: final report. --- */
    char report[128];
    int rlen = snprintf(report, sizeof(report),
                        "\n[backup_tool] wrote %zd bytes, read back %zd bytes\n",
                        (ssize_t)written, total_read);
    write(STDOUT_FILENO, report, (size_t)rlen);
    write_log("backup run complete");

    return EXIT_SUCCESS;
}
