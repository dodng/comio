#include "io_manager.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

int Io_Manager::get_io(std::string ip,int port)
{
	{
		AutoLock_Mutex auto_lock(&_io_dict_lock);
		//1.find
		Io_Key key(ip,port);
		std::map<Io_Key,Io_Value,Io_Comp>::iterator it = _io_dict.find(key);
		if ( it != _io_dict.end())
		{
			//2.if find ,update io_value
			if (get_sock_tcp_status(it->second._sd) == 0)
			{
				//check socket status  ok
				gettimeofday(&(it->second._last_use_time), 0);
				return it->second._sd;
			}
			else
			{
				//check socket status  fail
				close(it->second._sd);
				_io_dict.erase(it);
			}
		}

		//3.if not find or can not use,create a new one
		int sd = init_connet_fd(ip, port);
		if (sd <= 0)
		{return -1;}

		if (wait_3handshake(sd) != 0)
		{return -2;}

		struct timeval now_time = {0};
		gettimeofday(&now_time,0);
		Io_Value value(sd,now_time,now_time);
		//4.if connect and handshake ok,insert into io_value
		_io_dict.insert(std::make_pair(key,value));
		//5.return sd
		return sd;
	}
}

int Io_Manager::wait_3handshake(int fd)
{
	fd_set wset;
	int error;

	if (fd <= 0)
		return -1;

	FD_ZERO(&wset);
	FD_SET(fd,&wset);
	if(  select(fd+1, NULL, &wset, NULL,&_tv_con_timeout)  <= 0)
	{
		return -2;
	}
	else
	{
		error = get_sock_tcp_status(fd);
		if(error)
		{
			return -3;
		}
	}
	return 0;
}

int Io_Manager::init_connet_fd(std::string ip,int port)
{
	int ret = 0;
	int sd = 0;
	struct sockaddr_in peer_addr;
	//init peer_add
	bzero(&peer_addr,sizeof(peer_addr));
	peer_addr.sin_family = AF_INET;
	peer_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	peer_addr.sin_port = htons(port);

	//init sd

	if ((sd = socket(AF_INET,SOCK_STREAM,0) ) < 0)
	{
		ret = -1;
		return ret;
	}
	//init no_block socket
	if((fcntl(sd, F_SETFL, O_NONBLOCK)) < 0)
	{
		ret = -2;
		return ret;
	}
	//connect
	connect(sd,reinterpret_cast<sockaddr *>(&peer_addr),sizeof(peer_addr));

	return sd;
}

#if 0
int Io_Manager::close_io(std::string ip,int port)
{
	int ret = -1;
	{
		AutoLock_Mutex auto_lock(&_io_dict_lock);
		//1.find
		Io_Key key(ip,port);
		std::map<Io_Key,Io_Value,Io_Comp>::iterator it = _io_dict.find(key);
		if ( it != _io_dict.end())
		{
			close(it->second._sd);
			_io_dict.erase(it);
			ret = 0;
		}
	}
	return ret;

}
#endif
