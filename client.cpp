/*
    Based on original assignment by: Dr. R. Bettati, PhD
    Department of Computer Science
    Texas A&M University
    Date  : 2013/01/31
 */


#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#include <sys/time.h>
#include <cassert>
#include <assert.h>

#include <cmath>
#include <numeric>
#include <algorithm>

#include <list>
#include <vector>

#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "reqchannel.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
using namespace std;

Histogram hist;

struct req_args{
    BoundedBuffer* request_buffer;
    int reqNum;
    string name;
};

struct worker_args{
    RequestChannel* channel;
    BoundedBuffer* request_buffer;
    BoundedBuffer* john_buffer;
    BoundedBuffer* jane_buffer;
    BoundedBuffer* joe_buffer;
    
};

struct stat_args{
    int size;
    BoundedBuffer* stat_buffer;
    string name;

    // Histogram* hist;
};


void* request_thread_function(void* arg) {
	
    // cout<<"req function \n";

    struct req_args *req;
    req = (struct req_args*)arg;
	for(int i = 0; i < req->reqNum; i++) {
        //enqueue requests
        req -> request_buffer -> push(req->name);
	}
}

void* worker_thread_function(void* arg) {

    // cout<<"worker function \n";

    struct worker_args *req;
    req = (struct worker_args*)arg;
    BoundedBuffer* request_buffer = (BoundedBuffer* ) req->request_buffer;
	BoundedBuffer* res_buffer1 = (BoundedBuffer* ) req->john_buffer;
	BoundedBuffer* res_buffer2 = (BoundedBuffer* ) req->jane_buffer;
	BoundedBuffer* res_buffer3 = (BoundedBuffer* ) req->joe_buffer;

    RequestChannel* chan = req->channel;
	
 
    while(true) {
        string request = request_buffer->pop();
			req->channel->cwrite(request);
            
			if(request == "data John Smith") 
			{
                string response = chan->cread();
				res_buffer1->push(response);
            }
			else if(request == "data Jane Smith") 
			{
                string response = chan->cread();
				res_buffer2->push(response);
            }
			else if(request == "data Joe Smith") 
			{
                string response = chan->cread();
				res_buffer3->push(response);            
            }
			else
			{
			   	delete chan;
                break;
            }
    }
}

void* stat_thread_function(void* arg) {
  
    // cout<<"stat function \n";

    stat_args *hist_data = (stat_args*) arg;
    BoundedBuffer* buff = hist_data->stat_buffer;
    string request = "";
    string response = "";
    for(int i =0; i < hist_data->size; i++)
    {
        request = hist_data->name;
        response = buff->pop();
		// cout << "pushing a response " << response <<" ...request "<<request<< endl;

        hist.update(request,response);
    }
}

// Screen Refresh Handler
void screen_refresh(int sig, siginfo_t *si, void *uc)
{
    system("clear");
    hist.print ();        
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    int n = 100; //default number of requests per "patient"
    int w = 1; //default number of worker threads
    int b = 3 * n; // default capacity of the request buffer, you should change this default
    int opt = 0;
    RequestChannel::Type ipc_type =  RequestChannel::Type::MQ;
    while ((opt = getopt(argc, argv, "n:w:b:i:")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg); //This won't do a whole lot until you fill in the worker thread function
                break;
            case 'b':
                b = atoi (optarg);
                break;
            case 'i': {
                string ipc_typename(optarg);

                try {
                    ipc_type = RequestChannel::get_type(ipc_typename);
                } catch (const sync_lib_exception& e) {
                    cerr << e.what() << endl;
                    exit(1);
                }}
                break;
		}
    }
    // argv[0] is the name of the executable (including the path) that was run to begin this process.
        // key_t tok_key = ftok(argv[0], 'b');
        string path = argv[0];
        cout <<"\npath: "<< path <<endl<<endl;
    int pid = fork();
	if (pid == 0){
		execl("dataserver", (char*) NULL);

        // string ipc_typename = RequestChannel::get_typename(ipc_type);
        // execl("dataserver", "dataserver", ipc_typename.c_str(), key.c_str(), nullptr);
        // cerr << "Failed to launch dataserver: " << strerror(errno) << endl;

	}
	else {

        cout << "n == " << n << endl;
        cout << "w == " << w << endl;
        cout << "b == " << b << endl;
        cout << "i == " << RequestChannel::get_typename(ipc_type) << std::endl;

        
        /* BONUS */
    
        // Screen Refresh Signal Timer

        timer_t timerid;
        struct sigevent sev;
        struct itimerspec its;
        long long freq_nanosecs;
        sigset_t mask;
        struct sigaction sa;
        
        
        // Handler for Timer
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = screen_refresh;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM,&sa,NULL);
        
        // Block Timer Signal
        sigemptyset(&mask);
        sigaddset(&mask,SIGALRM);
        sigprocmask(SIG_SETMASK,&mask,NULL);
        
        // Create Timer
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGALRM;
        // sev.sigev_value.sival_ptr = &out;//timerid;
        timer_create(CLOCK_REALTIME, &sev, &timerid);
        
        freq_nanosecs = 2000000000; // 2 Second Timer
        its.it_value.tv_sec = freq_nanosecs / 1000000000;
        its.it_value.tv_nsec = freq_nanosecs % 1000000000;
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
        
        
        // ENABLE/DISABLE REFRESH HERE
        timer_settime(timerid, 0, &its, NULL);
        
        sigprocmask(SIG_UNBLOCK,&mask, NULL);
        ////////////////////////////////////////////////////////////////////////
        struct timeval start_time, end_time;

        gettimeofday(&start_time, NULL); //start time



        BoundedBuffer request_buffer(b);     
    struct req_args person[3];
            pthread_t req_threads[3];
            person[0].name="data John Smith";
            person[1].name="data Jane Smith";
            person[2].name="data Joe Smith";
            // string reqJane="data Jane Smith";
            // string reqJoe="data Joe Smith";
            for(int i = 0; i < 3; ++i) {
                person[i].request_buffer= &request_buffer;
                person[i].reqNum=n;
                pthread_create(&req_threads[i], NULL,request_thread_function,(void*)&person[i]);
                
            }
            cout << "Done populating request buffer" << endl;

            // RequestChannel::get_channel(ipc_type, "control", RequestChannel::CLIENT_SIDE, k);



            RequestChannel *chan = RequestChannel::get_channel(ipc_type, "control", RequestChannel::CLIENT_SIDE);
            BoundedBuffer request1_buffer(b/3);
            BoundedBuffer request2_buffer(b/3);
            BoundedBuffer request3_buffer(b/3);
            struct worker_args workers[w];
            pthread_t threads[w];


            for(int i=0; i<w;i++){
                chan->cwrite("newchannel");
                string s = chan->cread ();
cout << "\n\nFlag i: "<<i<<endl;

                cout << "\n\n main chan prompt: "<<s<<endl;
                RequestChannel *workerChannel = RequestChannel::get_channel(ipc_type, s, RequestChannel::CLIENT_SIDE);

                //spawn worker thread

                workers[i].request_buffer = &request_buffer;
                workers[i].channel = workerChannel;
                workers[i].john_buffer = &request1_buffer;
                workers[i].jane_buffer = &request2_buffer;
                workers[i].joe_buffer = &request3_buffer;

                pthread_create(&threads[i], NULL, worker_thread_function, (void*)&workers[i]);
                // threads.push_back(threads[i]);

            }

            pthread_t stat_thread1,stat_thread2,stat_thread3;
            stat_args stat1,stat2,stat3;

            stat1.stat_buffer = &request1_buffer;
            stat1.name = "data John Smith";
            stat1.size = n;

            stat2.stat_buffer = &request2_buffer;
            stat2.name = "data Jane Smith";
            stat2.size = n;

            stat3.stat_buffer = &request3_buffer;
            stat3.name = "data Joe Smith";
            stat3.size = n;

        pthread_create(&stat_thread1, NULL, &stat_thread_function, (void*) &stat1);
		pthread_create(&stat_thread2, NULL, &stat_thread_function, (void*) &stat2);
		pthread_create(&stat_thread3, NULL, &stat_thread_function, (void*) &stat3);
		
         
        //join all request threads
            for(int i=0; i<3;i++){
                pthread_join(req_threads[i],NULL);
            }
            for(int i = 0; i < w; ++i) {
                request_buffer.push("quit");
            }
            // cout << "done." << endl;


        //join all worker threads
            for(int i = 0; i < w; ++i) pthread_join(threads[i], NULL);
		


            chan->cwrite ("quit");
            delete chan;
     
        //join all stat threads
            // cout << "Joining stat threads !!!" << endl; 
            pthread_join(stat_thread1,NULL);
            pthread_join(stat_thread2,NULL);
            pthread_join(stat_thread3,NULL);
            
        cout << "All Done!!!" << endl; 
        gettimeofday(&end_time,NULL);
        

        if (timer_delete(timerid)) {
            cerr << "timer_delete error: " << strerror(errno) << endl;
        } else {
            cout << "Timer disarmed." << endl;
        }
        
        cout << "Results for n = " << n << ", w = " << w << ", b = " << b  <<  endl;
        system("clear");
        hist.print ();
        cout << "Sleeping..." <<  endl;
        usleep(10000);
        // chan->send_request("quit");
        

        double total_time = (end_time.tv_sec - start_time.tv_sec)*1000000 + (end_time.tv_usec - start_time.tv_usec);        
        cout << "Total run time: " << setprecision(7)<<total_time/1000000 << " secs" <<  endl;

        //This is to output the time results into time_results.txt for easy time collection
        ofstream myfile;
        myfile.open ("time_results_out.csv", ios_base::app);
        myfile  << n <<"," << w<<"," << b<<"," <<setprecision(7)<<total_time/1000000 << endl;
        // myfile << "Total run time: " << setprecision(7)<<total_time/1000000 << " secs" <<  endl<<  endl;

        // cout<< "\nTotal run time: "<<abs(end_time.tv_sec-start_time.tv_sec)<<" sec and "<<abs(end_time.tv_usec-start_time.tv_usec)<<" micro sec\n";

    }

}

