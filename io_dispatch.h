#ifndef _H_IO_DISPATCH
#define _H_IO_DISPATCH

#include "io_storage.h"
#include "io_manager.h"
#include "path_manager.h"
#include <pthread.h>
#include <sys/time.h>
#include <event.h>


struct io_dispatch_thread_info
{
	pthread_t thread_info;
	int thread_status;
	//for libevent
	struct event_base *p_event_base;
	//path_key and sd for accept
	Path_Key path_k;
	int path_k_sd;
	pthread_mutex_t * p_path_k_lock;
	//io_manager life is thread
	Io_Manager io_mgr; //life is one io_thread
	//path_manager life is mutlthreads
	Path_Manager * p_path_mgr; //life is one comio process
	
};

struct io_dispatch_info
{
	Io_Storage * p_upstream;
	Io_Storage * p_downstream;
	io_dispatch_thread_info * p_thread_info;
};

typedef void (* event_cb_func)(int fd, short int events, void *arg);
void ProcessDownStream_Recv(int fd, short int events, void *arg);
void ProcessUpStream_Recv(int fd, short int events, void *arg);

int io_dispatch_main();
#endif
