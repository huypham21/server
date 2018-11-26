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

	mkfifo(FIFO_NAME, 0644); // create

	printf("Waiting for readers...\n");
	fd = open(FIFO_NAME, O_WRONLY); //open
	if (fd < 0)
		return 0;
	printf("Got a reader--type some stuff\n");

	while (gets(s)) {
		if (!strcmp (s, "quit")) break;
		if ((num = write(fd, s, strlen(s))) == -1)
			perror("write");
		else
			printf("SENDER: wrote %d bytes\n", num);
	}
	//unlink (FIFO_NAME);

	return 0;
}
