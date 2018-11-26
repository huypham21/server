#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct my_msgbuf {
	long mtype;
	char mtext[200];
};

int main(void)
{
	struct my_msgbuf buf;
	key_t key = ftok ("a.txt", 100); // create a pseudo-random key
	int msqid = msgget(key, 0644| IPC_CREAT); // connect to the msg queueif (msqid == -1)
	if (msqid < 0){
		perror ("message queue error");
		return 0;; 		
	}
	while(1) { 
        if (msgrcv(msqid, &buf, sizeof buf.mtext, 0, 0)<= 0) {
            perror("msgrcv");
            exit(1);
        }
        printf("**********Received: %s\n", buf.mtext);
  }
	return 0;
}
