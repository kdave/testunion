#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

#ifndef SEEK_DATA
#define SEEK_DATA 3
#endif

static char buf[16*1024];

void do_test(const char *fname, loff_t data_start, loff_t len) {
        int fd;
        loff_t lret;

        fd = open(fname, O_RDWR|O_CREAT, 0644);
        if (fd == -1) {
                perror("open()");
                exit(1);
        }
        printf("[0,%llu] hole\n", data_start - 1);
        llseek(fd, data_start, SEEK_SET);
        write(fd, buf, len);
        printf("[%llu,%llu] data\n", data_start, len - 1);
        fsync(fd);
        llseek(fd, 0, SEEK_SET);
        lret = llseek(fd, 0, SEEK_DATA);
        printf("seek to data returned: %llu\n",
                        (unsigned long long)lret);
        close(fd);
}

int main() {
        do_test("seektest-small-even", 3072, 512);
        do_test("seektest-small-odd", 3072, 511);
        do_test("seektest-big-even", 8192, 512);
        do_test("seektest-big-odd", 8192, 511);
        do_test("seektest-big-aligned", 8192, 4096);
        do_test("seektest-big-aligned-1", 8192, 4095);
        do_test("seektest-big-aligned+1", 8192, 4097);

        return 0;
}
