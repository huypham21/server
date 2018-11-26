/* 
    File: requestchannel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2012/07/11

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>


#include <algorithm> //transform

#include "reqchannel.h"





const int MAX_MESSAGE = 255;

std::map<const std::string, RequestChannel::Type> RequestChannel::TypeMap = {
        {"FIFO", FIFO},
        {"MQ",   MQ},
        {"SHM",  SHM}
};


RequestChannel::RequestChannel(const string _name, const Side _side) :
my_name(_name), my_side(_side), side_name((_side == RequestChannel::SERVER_SIDE) ? "SERVER" : "CLIENT")
{
	// if (_side == SERVER_SIDE) {
	// 	open_write_pipe(pipe_name(WRITE_MODE).c_str());
	// 	open_read_pipe(pipe_name(READ_MODE).c_str());
	// }
	// else {
	// 	open_read_pipe(pipe_name(READ_MODE).c_str());
	// 	open_write_pipe(pipe_name(WRITE_MODE).c_str());
	// }
}

RequestChannel::~RequestChannel() {
	// close(wfd);
	// close(rfd);
	// //if (my_side == SERVER_SIDE) {
	// 	remove(pipe_name(READ_MODE).c_str());
	// 	remove(pipe_name(WRITE_MODE).c_str());
	//}
}


void EXITONERROR (string msg){
	perror (msg.c_str());
	exit (-1);
}

string RequestChannel::pipe_name(Mode _mode) {
	string pname = "fifo_" + my_name;

	if (my_side == CLIENT_SIDE) {
		if (_mode == READ_MODE)
			pname += "1";
		else
			pname += "2";
	}
	else {
	/* SERVER_SIDE */
		if (_mode == READ_MODE)
			pname += "2";
		else
			pname += "1";
	}
	return pname;
}

const string RequestChannel::get_typename(const Type& type) {

    for(const auto& ntp : TypeMap) {
        if(ntp.second == type)
            return ntp.first;
    }

    throw out_of_range("Unknown type: " + to_string(type));
}

const RequestChannel::Type RequestChannel::get_type(const std::string& type_name) {
    string typename_toUpper(type_name);
    transform(typename_toUpper.begin(), typename_toUpper.end(), typename_toUpper.begin(), ::toupper);
    try {
        return RequestChannel::TypeMap.at(typename_toUpper);
    } catch (const std::out_of_range&) {
        throw sync_lib_exception("Unknown IPC type: " + type_name);
    }
}

RequestChannel* RequestChannel::get_channel(const Type& type,const string& name,const Side& side) {
			cout<< "\n\n Type: " <<  get_typename(type);// << " ... Side: "<<side<<endl;
    switch(type) {
        case RequestChannel::Type::FIFO:
            return new FIFORequestChannel(name, side);break;
        case RequestChannel::Type::MQ:
            return new MQRequestChannel(name, side);break;
        // case RequestChannel::Type::SHM:
        //     return std::shared_ptr<RequestChannel>(new SHMRequestChannel(name, side, key));
        default:
            throw("Invalid request channel type");
    }
}

// std::string RequestChannel::send_request(std::string _request) {
// 	lock_guard<std::mutex> lock(send_request_lock);

//     if(cwrite(_request) < 0) {
//         return "ERROR";
//     }

//     std::string s = cread();
//     return s;
// }


/*--------------------------------------------------------------------------*/
/* FIFO implementation                                                      */
/*--------------------------------------------------------------------------*/


FIFORequestChannel::FIFORequestChannel(const string name, const Side side)
 : RequestChannel(name, side) {
	//Necessary for proper error handling
	sigset_t sigpipe_set;
	sigemptyset(&sigpipe_set);

	if(sigaddset(&sigpipe_set, SIGPIPE) < 0) {
		throw sync_lib_exception(my_name + ":" + side_name + ": failed on sigaddset");
	}
	
	if((errno = pthread_sigmask(SIG_SETMASK, &sigpipe_set, nullptr)) != 0) {
		throw sync_lib_exception(my_name + ":" + side_name + ": failed on pthread_sigmask");
	}
	
	if (side == SERVER_SIDE) {
		open_write_pipe(pipe_name(WRITE_MODE).c_str());
		open_read_pipe(pipe_name(READ_MODE).c_str());
	}
	else {
		open_read_pipe(pipe_name(READ_MODE).c_str());
		open_write_pipe(pipe_name(WRITE_MODE).c_str());
	}

}

FIFORequestChannel::~FIFORequestChannel() {
	close(wfd);
	close(rfd);

	if (my_side == RequestChannel::SERVER_SIDE) {
		if(false) {
            std::cout << "RequestChannel:" << my_name << ":" << side_name
                      << "close IPC mechanisms on server side for channel "
                      << my_name << std::endl;
        }

		/* Delete the underlying IPC mechanisms. */
		if (remove(pipe_name(READ_MODE).c_str()) != 0 && errno != ENOENT) {
			perror(std::string(my_name + ": Error deleting pipe read pipe").c_str());
		}

		if (remove(pipe_name(WRITE_MODE).c_str()) != 0 && errno != ENOENT) {
			perror(std::string(my_name + ": Error deleting pipe write pipe").c_str());
		}
	}
}

void FIFORequestChannel::create_pipe (string _pipe_name){
	mkfifo(_pipe_name.c_str(), 0600) < 0; //{
	//	EXITONERROR (_pipe_name);
	//}
}

void FIFORequestChannel::open_write_pipe(string _pipe_name) {
	
	cout << "\n\n _pipe_name " << _pipe_name <<endl;

	create_pipe (_pipe_name);

	wfd = open(_pipe_name.c_str(), O_WRONLY);
	if (wfd < 0) {
		EXITONERROR (_pipe_name);
	}

	// if(false) std::cout << my_name << ":" << side_name << ": done opening write pipe" << std::endl;
	// write_pipe_opened = true;
}

void FIFORequestChannel::open_read_pipe(string _pipe_name) {
	create_pipe (_pipe_name);
	rfd = open(_pipe_name.c_str (), O_RDONLY);
	if (rfd < 0) {
		perror ("");
		exit (0);
	}
}


string FIFORequestChannel::cread() {
    // std::lock_guard<std::mutex> lock(read_lock);

   char buf [MAX_MESSAGE];
	if (read(rfd, buf, MAX_MESSAGE) <= 0) {
		EXITONERROR ("cread");
	}
	string s = buf;
	return s;
}

int FIFORequestChannel::cwrite(string msg) {
	int write_return_value;

    // std::lock_guard<std::mutex> lock(write_lock);
	if (msg.size() > MAX_MESSAGE) {
		EXITONERROR ("cwrite");
	}
	if (write(wfd, msg.c_str(), msg.size()+1) < 0) { // msg.size() + 1 to include the NULL byte
		EXITONERROR ("cwrite");
	}
	// write_return_value = write(wfd, msg.c_str(), msg.size()+1);
	return 0;
	// return write_return_value;

}



/*--------------------------------------------------------------------------*/
/* Message Queue implementation                                                      */
/*--------------------------------------------------------------------------*/
struct my_msgbuf {
	long mtype;
	char mtext[200];
};

int MQRequestChannel::chan_count = 0;
MQRequestChannel::MQRequestChannel(const string _name, const Side _side)
: RequestChannel(_name, _side)
{


	MQRequestChannel::chan_count++;

	cout << "\n\n chan_count: "<<chan_count<<endl;
	key_t clientKey;
	key_t serverKey; 
	if( clientKey = ftok (".", 'a')==-1){
		perror("ftok");
		exit(1); // create a pseudo-random key
	}
	if( serverKey = ftok (".", 'a') == -1){
		// create a pseudo-random key
		perror("ftok");
		exit(1); // create a pseudo-random key
	}

	serverID = msgget(clientKey, 0666 | IPC_CREAT); // create the msg queue
	clientID = msgget(serverKey, 0666 | IPC_CREAT); // create the msg queue

	// if (serverID < 0){
	// 	perror ("message queue error");	
	// }

	// if (clientID < 0){
	// 	perror ("message queue error");	
	// }
}

MQRequestChannel::~MQRequestChannel() {

	msgctl(serverID, IPC_RMID, NULL);
	msgctl(clientID, IPC_RMID, NULL);
}

std::string MQRequestChannel::cread() {


	struct my_msgbuf server_buf;
	struct my_msgbuf client_buf;

server_buf.mtype = 1;
client_buf.mtype = 1;


	if (side_name=="SERVER"){
		//int serverID = msgget(serverKey, 0644| IPC_CREAT); // connect to the msg queueif (msqid == -1)
		if (serverID < 0){
			perror ("message queue error");		
		}
		int server_read_return_value = msgrcv(clientID, &server_buf, sizeof(server_buf.mtext), 0, 0);
    	if (server_read_return_value<= 0) {
        	perror("msgrcv");
        	exit(1);
    	}
		std::string s1 = server_buf.mtext;
		return s1;
	}

	if (side_name=="CLIENT"){
		//int clientID = msgget(clientKey, 0644| IPC_CREAT); // connect to the msg queueif (msqid == -1)
		if (clientID < 0){
			perror ("message queue error");
		}
		int client_read_return_value = msgrcv(serverID, &client_buf, sizeof(client_buf.mtext), 0, 0);
    	if (client_read_return_value<= 0) {
        	perror("msgrcv");
        	exit(1);
    	}
		std::string s2 = client_buf.mtext;
		return s2;
	}

}

int MQRequestChannel::cwrite(std::string _msg) {



	struct my_msgbuf server_buf;
	struct my_msgbuf client_buf;

	server_buf.mtype = 1;
	client_buf.mtype = 1;

	int server_write_return_value;
	int client_write_return_value;


	if (side_name=="SERVER"){
		//int serverID = msgget(serverKey, 0644 | IPC_CREAT); // create the msg queue
		const char * s1 = _msg.c_str();
		//std::cout << _msg << std::endl;
		strcpy(server_buf.mtext, s1);	
		server_write_return_value = msgsnd(serverID, &server_buf, strlen(s1)+1 , 0);
		return server_write_return_value;
	}

	if (side_name=="CLIENT"){
		//int clientID = msgget(clientKey, 0644 | IPC_CREAT); // create the msg queue
		const char * s2 = _msg.c_str();
		//std::cout << _msg << std::endl;
		strcpy(client_buf.mtext, s2);		
		client_write_return_value = msgsnd(clientID, &client_buf, strlen(s2)+1 , 0);
		return client_write_return_value;
	}
}