#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_MULTIPLIER 8  // 可根据实验调整

long io_blocksize(int fd) {
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) pagesize = 4096;
    struct stat st;
    long blksize = 0;
    if (fstat(fd, &st) == 0) {
        blksize = st.st_blksize;
    }
    if (blksize <= 0) blksize = 4096;
    long base = (pagesize > blksize) ? pagesize : blksize;
    return base * BUF_MULTIPLIER;
}

void* align_alloc(size_t size) {
    void* ptr = NULL;
    long pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize <= 0) pagesize = 4096;
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
    // fadvise
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) != 0) {
        perror("posix_fadvise");
        // 不是致命错误，继续
    }
    long bufsize = io_blocksize(fd);
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