/***************************************************************************
 * 
 *	author:dodng
 *	e-mail:dodng12@163.com
 * 	2017/3/16
 *   
 **************************************************************************/
 



#ifndef  __CONFIG_H_
#define  __CONFIG_H_

#include "http.h"
#include "policy_interface.h"
#include <sys/time.h>
#include "common.h"

#define UPDATE_PROCESS_THREAD 1

#define BUFF_SIZE (1024 * 1024)

enum status_code
{
    status_init = 0,
    status_recv = 1,
    status_recv_done = 2,
    status_send = 3,
    status_send_done = 4,
    status_process = 5,
    status_process_done = 6
};

void RecvData(int fd, short int events, void *arg);
void SendData(int fd, short int events, void *arg);

typedef void (* event_cb_func)(int fd, short int events, void *arg);

struct sig_ev_arg 
{
	void *ptr;
	int listen_fd;
	
};


class interface_data
{
	public:
		interface_data(int recv_buff_size,int send_buff_size):recv_buff_len(recv_buff_size),send_buff_len(send_buff_size)
		{
			recv_buff = 0;
			send_buff = 0;
			now_send_len = 0;
			now_recv_len = 0;

			status = status_init;
			sd = -1;
			recv_buff = new char[recv_buff_len];
			send_buff = new char[send_buff_len];

		}
		~interface_data()
		{
			if (0 != recv_buff) {
				delete []recv_buff;
				recv_buff = 0;
			}
			if (0 != send_buff) {
				delete []send_buff;
				send_buff = 0;
			}
		}

		void reset()
		{
			now_send_len = 0;
			now_recv_len = 0;
			policy.reset();
			http.reset();
		}
		//data
		char *recv_buff;
		char *send_buff;
		int recv_buff_len;
		int send_buff_len;
		int now_recv_len;
		int now_send_len;
		struct event ev;
		int status;
		int sd;
        struct timeval l_time[6];
		http_entity http;
		policy_entity policy;
};

#endif  //__CONFIG_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
