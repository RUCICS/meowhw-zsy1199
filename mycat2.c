#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

long io_blocksize() {
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) {
        // fallback to 4096 if error
        return 4096;
    }
    return pagesize;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    long bufsize = io_blocksize();
    char *buf = malloc(bufsize);
    if (!buf) {
        perror("malloc");
        close(fd);
        return 1;
    }
    ssize_t n;
    while ((n = read(fd, buf, bufsize)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w < 0) {
                perror("write");
                free(buf);
                close(fd);
                return 1;
            }
            written += w;
        }
    }
    if (n < 0) {
        perror("read");
        free(buf);
        close(fd);
        return 1;
    }
    free(buf);
    close(fd);
    return 0;
} 