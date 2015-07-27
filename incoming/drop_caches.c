#include <unistd.h>
#include <fcntl.h>

int main() {
	int fd;

	sync();
	sync();
	if((fd=open("/proc/sys/vm/drop_caches", O_WRONLY)) < 0) {
		perror("open");
		return 1;
	}

	if(write(fd, "3", 1) < 0) {
		perror("write");
		return 1;
	}

	return 0;
}
