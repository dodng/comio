#ifndef _H_PATH_MANANGER
#define _H_PATH_MANANGER

#include <map>
#include <string>
#include "common.h"
#include <pthread.h>
#include <vector>
#include <stdint.h>
#include <unistd.h>

struct Path_Key
{
	uint32_t _port;
	Path_Key(): _port(0){}
	Path_Key(int port){ _port = (uint32_t)port;}
};

struct Path_Value
{
	std::string _ip;
	uint32_t _port;
	int _ratio_config;
	int _ratio_use;
	Path_Value(): _ip(""),_port(0),_ratio_config(0),_ratio_use(0){}
	Path_Value(std::string ip,int port,int ratio_config)
	{
		_ip = ip;
		_port = (uint32_t)port;
		_ratio_config = ratio_config;
		_ratio_use = _ratio_config;
	}
};

struct Path_Key_Accept_Sock
{
	int _sd;
	pthread_mutex_t * _sd_lock;
	int use_num;
};

struct Path_Comp  
{  

	typedef Path_Key value_type;  

	bool operator () (const value_type & ls, const value_type &rs)  
	{  
		return ls._port < rs._port;  
	}  

}; 

class Path_Manager
{
	public:
		Path_Manager()
			{
			pthread_mutex_init(&_path_dict_lock,NULL);
			pthread_mutex_init(&_accept_dict_lock,NULL);
			pthread_mutex_init(&_path_ptr_dict_lock,NULL);
			}
		
		~Path_Manager()
			{
			pthread_mutex_destroy(&_path_dict_lock);
			pthread_mutex_destroy(&_accept_dict_lock);
			pthread_mutex_destroy(&_path_ptr_dict_lock);
			}
		
		bool Update_Path(Path_Key in_key,std::vector<Path_Value> & in_value_vec);
		bool Select_Path(Path_Key in_key,Path_Value & out_value);
		void Delete_Path(Path_Key in_key);
		bool Get_Path_Listen_Sock(Path_Key in_key, Path_Key_Accept_Sock & out_accept_sock);
		void Put_Path_Listen_Sock(Path_Key in_key);
		void Add_Path_Ptr(Path_Key in_key, void *p_str);
		void  *Del_Path_Ptr(Path_Key in_key);
		
	private:
		std::map<Path_Key,std::vector<Path_Value>,Path_Comp> _path_dict;
		pthread_mutex_t _path_dict_lock;
		std::map<Path_Key,Path_Key_Accept_Sock,Path_Comp> _accept_dict;
		pthread_mutex_t _accept_dict_lock;
		std::map<Path_Key,std::vector<void *>,Path_Comp> _path_ptr_dict;
		pthread_mutex_t _path_ptr_dict_lock;
};
#endif
