
#ifndef _reqchannel_H_                   
#define _reqchannel_H_
// #define DEFAULT_KEY "ipc_"

#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <map>


class sync_lib_exception : public std::exception {
	std::string err = "failure in sync library";
	
public:
	sync_lib_exception() {}
	sync_lib_exception(std::string msg) : err(msg) {}
	virtual const char* what() const throw() {
		return err.c_str();
	}
};

using namespace std;

void EXITONERROR (string msg);

class RequestChannel {

public:

	typedef enum {SERVER_SIDE, CLIENT_SIDE} Side;

	typedef enum {READ_MODE, WRITE_MODE} Mode;

	typedef enum {FIFO, MQ, SHM} Type;

    static map<const string, Type> TypeMap;

protected:

    // const Type curr_type;
	string pipe_name(Mode _mode);

    // string  base_key = "";
	string   my_name = "";
	string side_name = "";
	Side     my_side;

	/*  The current implementation uses named pipes. */ 
	mutex read_lock, write_lock, send_request_lock;
	
public:

	/* -- CONSTRUCTOR/DESTRUCTOR */

	RequestChannel(const string _name, const Side _side);
	//(const string _name, const Side _side);
	/* Creates a "local copy" of the channel specified by the given name. 
	 If the channel does not exist, the associated IPC mechanisms are 
	 created. If the channel exists already, this object is associated with the channel.
	 The channel has two ends, which are conveniently called "SERVER_SIDE" and "CLIENT_SIDE".
	 If two processes connect through a channel, one has to connect on the server side 
	 and the other on the client side. Otherwise the results are unpredictable.

	 NOTE: If the creation of the request channel fails (typically happens when too many
	 request channels are being created) and error message is displayed, and the program
	 unceremoniously exits.

	 NOTE: It is easy to open too many request channels in parallel. In most systems,
	 limits on the number of open files per process limit the number of established
	 request channels to 125.
	*/

	~RequestChannel();
	/* Destructor of the local copy of the bus. By default, the Server Side deletes any IPC 
	 mechanisms associated with the channel. */


	/* A factory constructor method */
	static RequestChannel* get_channel(const Type& type,const string& name,const Side& side);

	/* Returns the name corresponding to a type */
	static const std::string get_typename(const Type& type);

    /* Returns the type corresponding to a type name */
    static const Type get_type(const string& type_name);

    /* Send a string over the channel and wait for a reply. */
	string send_request(string _request);

	// === GETTER FUNCTIONS ===

    /* Returns the name corresponding to the name of the current instance */
    string name() { return my_name; }


    // === VIRTUAL FUNCTIONS ===

    /* Blocking read of data from the channel. Returns a string of characters read from the channel. */
	virtual std::string cread() = 0;

    /* Write the data to the channel. The function returns the number of characters written to the channel. */
	virtual int cwrite(string msg) = 0;
};


class FIFORequestChannel : public RequestChannel {
private:
    int wfd;
    int rfd;

	void create_pipe (string _pipe_name);
    void open_read_pipe(string _pipe_name);
    void open_write_pipe(string _pipe_name);
    bool read_pipe_opened = false;
    bool write_pipe_opened = false;

public:

    // === CON/DESTRUCTORS ===

	FIFORequestChannel(const std::string _name, const Side _side);
    ~FIFORequestChannel();

    // === OVERRIDDEN FUNCTIONS ===

    string cread() override;
    int cwrite(string msg) override;

    // === FIFO-SPECIFIC FUNCTIONS ===

    /* Returns the file descriptor used to read from the channel. */
    int read_fd() const { return rfd; }

    /* Returns the file descriptor used to write to the channel. */
    int write_fd() const { return wfd; }
};

class MQRequestChannel : public RequestChannel {
private:
    int wfd;
    int rfd;
	string path;
	// void create_pipe (string _pipe_name);
    // void open_read_pipe(string _pipe_name);
    // void open_write_pipe(string _pipe_name);
    // bool read_pipe_opened = false;
    // bool write_pipe_opened = false;

	int serverID;
	int clientID;


	pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutexattr_t srl_attr;
	pthread_mutex_t send_request_lock;

public:
	static int chan_count;

    // === CON/DESTRUCTORS ===

	MQRequestChannel(const string _name, const Side _side);
    ~MQRequestChannel();

    // === OVERRIDDEN FUNCTIONS ===

    string cread() override;
    int cwrite(string msg) override;
	void set_path(string key){}
    // === FIFO-SPECIFIC FUNCTIONS ===

    /* Returns the file descriptor used to read from the channel. */
    int read_fd() const { return rfd; }

    /* Returns the file descriptor used to write to the channel. */
    int write_fd() const { return wfd; }
};

/*--------------------------------------------------------------------------*/
/* CLASS   s e m a p h o r e  */
/*--------------------------------------------------------------------------*/

// class semaphore_exception : public std::exception {
//     std::string err = "failure in sync library";

// public:
//     semaphore_exception() {}
//     semaphore_exception(std::string msg) : err(msg) {}

//     virtual const char* what() const throw() {
//         return err.c_str();
//     }
// };


// class SemaphoreBase {

// public:

//     /* -- CONSTRUCTOR/DESTRUCTOR */

//     SemaphoreBase(const unsigned long long& _val = 0) {};
//     virtual ~SemaphoreBase() {};
//     /* Clearer names for the functions */

//     virtual void notify() = 0;
//     virtual void wait() = 0;
//     virtual void destroy() {};

//     /* -- SEMAPHORE OPERATIONS */

//     inline void P() { wait(); }

//     inline void V() { notify(); }
// };



// class KernelSemaphore : public SemaphoreBase {
// private:
//     sem_t *handle;
//     string name;
// public:
//     KernelSemaphore(const std::string& name, const unsigned long long& _val = 0) : name(name) {
//         if((handle = sem_open(("/" + name).c_str(), O_RDWR | O_CREAT, 0600, _val)) == SEM_FAILED)
//             throw semaphore_exception("Error opening semaphore '" + name + "': " + strerror(errno));
//     }

//     ~KernelSemaphore() {
//         sem_close(handle);
//     }

//     void destroy() override {
//         if(sem_unlink(("/" + name).c_str()) == -1)
//             throw semaphore_exception("Error destroying semaphore '" + name + "': " + strerror(errno));
//     }

//     void notify() override {
//         if(sem_post(handle) == -1)
//             throw semaphore_exception("Error notifying semaphore '" + name + "': " + strerror(errno));
//     }

//     void wait() override {
//         if(sem_wait(handle) == -1)
//             throw semaphore_exception("Error waiting semaphore '" + name + "': " + strerror(errno));
//     }
// };

#endif


