#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

long io_blocksize() {
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) {
        return 4096;
    }
    return pagesize;
}

void* align_alloc(size_t size) {
    void* ptr = NULL;
    long pagesize = io_blocksize();
    if (posix_memalign(&ptr, (size_t)pagesize, size) != 0) {
        return NULL;
    }
    return ptr;
}

void align_free(void* ptr) {
    free(ptr);
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
    char *buf = align_alloc(bufsize);
    if (!buf) {
        perror("align_alloc");
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
                align_free(buf);
                close(fd);
                return 1;
            }
            written += w;
        }
    }
    if (n < 0) {
        perror("read");
        align_free(buf);
        close(fd);
        return 1;
    }
    align_free(buf);
    close(fd);
    return 0;
} 