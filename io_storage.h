#ifndef _H_IO_STORAGE
#define _H_IO_STORAGE

#include <event.h>

enum status_code
{
    status_init = 0,
    status_reset = 1
};

class Io_Storage
{
public:
	Io_Storage(int recv_buff_len);
	~Io_Storage();

	void reset();
	void reset_data();
	//0:ok ,else return -1
	int set_io_sd(int sd);

	//return data_len >0:ok,else fail
	int process_recv_event(int recv_sd);
	int send_2_peer(int peer_sd);

	int io_status;
	//io Scheduling
	int io_sd;
	struct event _ev;
private:
	//data
	char * _recv_buff;
	int _recv_buff_len;
	int _now_recv_len;
};

#endif
