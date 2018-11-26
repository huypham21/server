#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 1024  /* make it a 1K shared memory segment */

int main(int argc, char *argv[])
{
	key_t key;
	int shmid;
	char *data;
	/* make the key: */
	if ((key = ftok("a.txt", 200)) == -1) {
		perror("ftok");
		exit(1);
	}
	/* connecting to the segment: */
	if ((shmid = shmget(key, SHM_SIZE, 0644)) == -1) {
		perror("shmget");
		exit(1);
	}
	/* attach to the segment to get a pointer to it: */
	data = shmat(shmid, 0, 0);
	if (data == -1) {
		perror("shmat");
		exit(1);
	}
	/* read from the shared-memory segment: */
	printf("***********segment contains: \"%s\"\n", data);
	/* detach from the segment: */
	if (shmdt(data) == -1) {
		perror("shmdt");
		exit(1);
	}
	return 0;
}