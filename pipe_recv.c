#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "test.txt"

int main(void)
{
	char s[300];
	int num, fd;

	printf("waiting for writers...\n");
	fd = open(FIFO_NAME, O_RDONLY); // connect
	printf("got a writer\n");

	do {
		if ((num = read(fd, s, sizeof(s))) == -1)
			perror("read");
		else {
			s[num] = '\0';
			printf("RECV: read %d bytes: \"%s\"\n", num, s);
		}
	} while (num > 0);
	return 0;
}
