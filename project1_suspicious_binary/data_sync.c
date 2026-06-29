/*
 * data_sync.c  --  A simple file-synchronisation tool (simulation).
 *
 * This program is the "suspicious binary" used in Project 1. It is a small,
 * benign utility that mirrors regular files from a SOURCE directory into a
 * DESTINATION directory, copying a file only when it is missing in the
 * destination or when the source copy is newer (a one-way rsync-style sync).
 *
 * It deliberately uses a broad spread of C library facilities so that the
 * compiled ELF binary contains a rich set of relocations / imported symbols
 * (directory traversal, stat(2), buffered file I/O, dynamic memory and string
 * handling). That makes the static-analysis exercise (objdump / nm / readelf)
 * meaningful: the imported functions alone reveal what the program "likely
 * does" without ever running it.
 *
 * Build:   gcc -O0 -g -o data_sync data_sync.c
 * Usage:   ./data_sync <source_dir> <dest_dir>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define COPY_BUF_SIZE 4096

/* Join "dir" and "name" into "out" as dir/name, bounded by out_size. */
static void join_path(char *out, size_t out_size, const char *dir,
                      const char *name)
{
    snprintf(out, out_size, "%s/%s", dir, name);
}

/* Copy a single regular file from src to dst using a fixed buffer. */
static int copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    if (in == NULL) {
        fprintf(stderr, "  ! cannot open source %s: %s\n", src,
                strerror(errno));
        return -1;
    }

    FILE *out = fopen(dst, "wb");
    if (out == NULL) {
        fprintf(stderr, "  ! cannot open dest %s: %s\n", dst, strerror(errno));
        fclose(in);
        return -1;
    }

    char *buffer = malloc(COPY_BUF_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "  ! out of memory\n");
        fclose(in);
        fclose(out);
        return -1;
    }

    size_t n;
    size_t total = 0;
    while ((n = fread(buffer, 1, COPY_BUF_SIZE, in)) > 0) {
        if (fwrite(buffer, 1, n, out) != n) {
            fprintf(stderr, "  ! write error on %s\n", dst);
            free(buffer);
            fclose(in);
            fclose(out);
            return -1;
        }
        total += n;
    }

    free(buffer);
    fclose(in);
    fclose(out);
    printf("  + copied %s (%zu bytes)\n", dst, total);
    return 0;
}

/*
 * Decide whether src needs to be synced to dst.
 * Returns 1 if dst is missing or src is strictly newer, 0 otherwise.
 */
static int needs_sync(const char *src, const char *dst)
{
    struct stat ss, ds;

    if (stat(src, &ss) != 0)
        return 0;                 /* cannot stat source -> skip */

    if (stat(dst, &ds) != 0)
        return 1;                 /* dst missing -> must copy   */

    return (ss.st_mtime > ds.st_mtime) ? 1 : 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_dir> <dest_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[1];
    const char *dst_dir = argv[2];

    /* Ensure the destination directory exists (mode 0755). */
    if (mkdir(dst_dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Cannot create dest dir %s: %s\n", dst_dir,
                strerror(errno));
        return EXIT_FAILURE;
    }

    DIR *dir = opendir(src_dir);
    if (dir == NULL) {
        fprintf(stderr, "Cannot open source dir %s: %s\n", src_dir,
                strerror(errno));
        return EXIT_FAILURE;
    }

    time_t start = time(NULL);
    printf("[data_sync] synchronising %s -> %s\n", src_dir, dst_dir);

    int copied = 0, skipped = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip "." and ".." */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char src_path[1024];
        char dst_path[1024];
        join_path(src_path, sizeof(src_path), src_dir, entry->d_name);
        join_path(dst_path, sizeof(dst_path), dst_dir, entry->d_name);

        /* Only sync regular files. */
        struct stat st;
        if (stat(src_path, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        if (needs_sync(src_path, dst_path)) {
            if (copy_file(src_path, dst_path) == 0)
                copied++;
        } else {
            printf("  = up to date %s\n", dst_path);
            skipped++;
        }
    }

    closedir(dir);

    time_t end = time(NULL);
    printf("[data_sync] done in %.0f s: %d copied, %d skipped\n",
           difftime(end, start), copied, skipped);

    return EXIT_SUCCESS;
}
