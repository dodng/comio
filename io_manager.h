#ifndef _H_IO_MANAGER
#define _H_IO_MANAGER

#include <string>
#include <string.h>
#include "common.h"
#include <map>
#include "pthread.h"
#include <sys/time.h>
#include <unistd.h>

struct Io_Key
{
	std::string _ip;
	int _port;
	Io_Key(std::string ip,int port)
	{
		_ip = ip;
		_port = port;
	}
};

struct Io_Value
{
	int _sd;
	struct timeval _create_time;
	struct timeval _last_use_time;
	Io_Value(int sd,struct timeval create_time,struct timeval last_use_time)
	{
		_sd =sd;
		_create_time = create_time;
		_last_use_time = last_use_time;
	}
};

struct Io_Comp
{

        typedef Io_Key value_type;

        bool operator () (const value_type & ls, const value_type &rs)
        {
		int ret = strncmp(ls._ip.c_str(),rs._ip.c_str(),ls._ip.size()>rs._ip.size()?rs._ip.size():ls._ip.size());

		if (ret == 0)
			return ls._port < rs._port;
		else if (ret < 0)
			return true;
		else 
			return false;
        }

};

class Io_Manager
{
	public:

		Io_Manager(struct timeval tv_con_timeout) 
		{
			_tv_con_timeout = tv_con_timeout;
			pthread_mutex_init(&_io_dict_lock,NULL);
		}
		Io_Manager()
		{
			_tv_con_timeout.tv_sec = 0;
			_tv_con_timeout.tv_usec = 10000;
			
			pthread_mutex_init(&_io_dict_lock,NULL);
		}
		~Io_Manager() 
		{
			//release all sd
			for (std::map<Io_Key,Io_Value,Io_Comp>::iterator it = _io_dict.begin() ;
					it != _io_dict.end();
					++it)
			{
				close(it->second._sd);
			}

			pthread_mutex_destroy(&_io_dict_lock);
		}
		int get_io(std::string ip,int port); //return can use sd,ok:>0,else:error
//		int close_io(std::string ip,int port); //ok:0,else:error

	private:
		int init_connet_fd(std::string ip,int port); //return can use sd,ok:>0,else:error
		int wait_3handshake(int fd); //ok:0,else:error

		struct timeval _tv_con_timeout;
		std::map<Io_Key,Io_Value,Io_Comp> _io_dict;
		pthread_mutex_t _io_dict_lock;

};
#endif
