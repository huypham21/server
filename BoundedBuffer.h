#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <pthread.h>
#include <queue>
#include <string>
using namespace std;

class BoundedBuffer {
private:
	queue<string> q;

    int n;

	pthread_mutex_t mtx;
    pthread_cond_t full, empt;

public:
    BoundedBuffer(int);
	~BoundedBuffer();
	int size();
    void push (string);
    string pop();

};

#endif /* BoundedBuffer_ */
