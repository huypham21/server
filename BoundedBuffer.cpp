#include "BoundedBuffer.h"
#include <string>
#include <queue>
using namespace std;

BoundedBuffer::BoundedBuffer(int _cap) {
	n=_cap; 
	pthread_mutex_init(&mtx,NULL);
	pthread_cond_init(&full,NULL);
	pthread_cond_init(&empt,NULL);
}

BoundedBuffer::~BoundedBuffer() {
	queue<string>z;
	swap(q,z);
	pthread_mutex_destroy(&mtx);
	pthread_cond_destroy(&full);
	pthread_cond_destroy(&empt);
}

int BoundedBuffer::size() {
	pthread_mutex_lock(&mtx);
	int z = q.size();
	pthread_mutex_unlock(&mtx);
	return z;
}

void BoundedBuffer::push(string str) {
	/*
	Is this function thread-safe??? Does this automatically wait for the pop() to make room 
	when the buffer if full to capacity???
	*/
	pthread_mutex_lock(&mtx);
	// count--;
	// if (count < 0) {
	// 	pthread_cond_wait(&q, &mtx);
	// }
	
	while(q.size()==n){
		pthread_cond_wait(&full, &mtx);
	}
	q.push (str);
	//send signal to Producer
	pthread_cond_signal(&empt);
	
	pthread_mutex_unlock(&mtx);
}

string BoundedBuffer::pop() {
	/*
	Is this function thread-safe??? Does this automatically wait for the push() to make data available???
	*/
	pthread_mutex_lock(&mtx);
	
	// count++;
	// 	if (count <= 0) {
	// 		pthread_cond_signal(&q);
	// 	}

	while(q.size()==0){
		pthread_cond_wait(&empt, &mtx);
	}
	string s = q.front();
	q.pop();
	pthread_cond_signal(&full);

	pthread_mutex_unlock(&mtx);
	return s;
}