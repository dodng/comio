#ifndef _H_IO_DISPATCH
#define _H_IO_DISPATCH

#include "io_storage.h"
#include "io_manager.h"
#include "path_manager.h"
#include <pthread.h>
#include <sys/time.h>
#include <event.h>

enum thread_code
{
    thread_use = 0,
    thread_closing = 1,
    thread_close = 2
};

//lifestyle is one thread
struct io_dispatch_thread_info
{
	pthread_t thread_info;
	int thread_status;
	//for libevent
	struct event_base *p_event_base;
	struct event timer_ev;
	struct timeval timer_tv;
	
	//path_key and sd for accept
	Path_Key path_k;
	int path_k_sd;
	pthread_mutex_t * p_path_k_lock;
	//io_manager life is thread
	Io_Manager io_mgr; //life is one io_thread
	//path_manager life is mutlthreads
	Path_Manager * p_path_mgr; //life is one comio process
	
};

//lifestyle is one io process
struct io_dispatch_info
{
	io_dispatch_info(int upstream_buff_size,int downstream_buff_size)
	{
		p_upstream = new Io_Storage(upstream_buff_size);
		p_downstream = new Io_Storage(downstream_buff_size);
	}
	~io_dispatch_info()
	{
		if (p_upstream) {delete p_upstream;}
		if (p_downstream) {delete p_downstream;}
	}
	Io_Storage * p_upstream;
	Io_Storage * p_downstream;
	io_dispatch_thread_info * p_thread_info;
	Path_Value path_res;
	struct timeval l_time[8];//clock time
};

typedef void (* event_cb_func)(int fd, short int events, void *arg);
void ProcessDownStream_Recv(int fd, short int events, void *arg);
void ProcessUpStream_Recv(int fd, short int events, void *arg);
void ProcessTimer(int fd, short int events, void *arg);


//interface for other module
int io_launch_one_thread(Path_Key & in_key, Path_Manager & in_path_mgr);

#endif
