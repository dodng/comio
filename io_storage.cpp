#include "io_storage.h"
#include <sys/types.h>
#include <sys/socket.h>

Io_Storage::Io_Storage(int recv_buff_len):_recv_buff_len(recv_buff_len)
{
	io_status = status_init;
	io_sd = -1;
	_recv_buff = new char[_recv_buff_len];
}

Io_Storage::~Io_Storage()
{
	if (0 != _recv_buff)
	{
		delete []_recv_buff;
		_recv_buff = 0;
	}

}

void Io_Storage::reset()
{
	io_status = status_reset;
	_now_recv_len = 0;
	io_sd = 0;
}

void Io_Storage::reset_data()
{
	io_status = status_reset;
	_now_recv_len = 0;
}

int Io_Storage::set_io_sd(int sd)
{
	if (sd > 0)
	{io_sd = sd;}
	return 0;
}

int Io_Storage::process_recv_event(int recv_sd)
{
	if (recv_sd <= 0) { return -1;}

	char *buff = (char *)(_recv_buff) + _now_recv_len;
	int buff_len = _recv_buff_len - _now_recv_len;

	int ret = -3;
	ret = recv(recv_sd,buff, buff_len, 0);
	if (ret > 0)
	{
		_now_recv_len += ret;
	}

	return ret;
}


int Io_Storage::send_2_peer(int peer_sd)
{
	if (peer_sd <= 0) {return -1;}

	if (_now_recv_len <= 0) {return -2;}

	int ret = -2;
	ret = send(peer_sd, _recv_buff, _now_recv_len, MSG_NOSIGNAL);
	_now_recv_len = 0;

	return ret;
}
