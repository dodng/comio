#include "path_manager.h"

bool Path_Manager::Update_Path(Path_Key in_key,std::vector<Path_Value> & in_value_vec)
{
	if (in_value_vec.size() <= 0 ) {return false;}

	{
		AutoLock_Mutex auto_lock(&_path_dict_lock);

		//1.find key
		std::map<Path_Key,std::vector<Path_Value>,Path_Comp>::iterator it = _path_dict.find(in_key);
		if ( it != _path_dict.end())
		{
			//2.delete key
			_path_dict.erase(it);
		}
		//3.insert key
		_path_dict.insert(std::make_pair(in_key,in_value_vec));
	}
	return true;
}

bool Path_Manager::Select_Path(Path_Key in_key,Path_Value & out_value)
{
	{
		AutoLock_Mutex auto_lock(&_path_dict_lock);
		//1.find key
		std::map<Path_Key,std::vector<Path_Value>,Path_Comp>::iterator it = _path_dict.find(in_key);
		if ( it == _path_dict.end()) //not found
		{
			return false;
		}
		else
		{
			int max_ratio_pos = 0;
			int max_ratio = 0;
			int sum_ratio = 0;
			//2.loop vector 1,find sum_ratio max_ratio
			for (int i = 0 ;i < it->second.size() ;i++)
			{
				sum_ratio += it->second[i]._ratio_config;
				if (i == 0 || it->second[i]._ratio_use > max_ratio)
				{
					max_ratio = it->second[i]._ratio_use;
					max_ratio_pos = i;
				}
			}
			//3.get the path
			out_value = it->second[max_ratio_pos];

			//4.loop vector 2,update ratio_use
			for (int i = 0 ;i < it->second.size() ;i++)
			{
				if (i == max_ratio_pos) 
				{it->second[i]._ratio_use -= sum_ratio;}

				it->second[i]._ratio_use += it->second[i]._ratio_config;
			}
		}
	}
	return true;
}

void Path_Manager::Delete_Path(Path_Key in_key)
{
	{
		AutoLock_Mutex auto_lock(&_path_dict_lock);

		//1.find key
		std::map<Path_Key,std::vector<Path_Value>,Path_Comp>::iterator it = _path_dict.find(in_key);
		if ( it != _path_dict.end())
		{
			_path_dict.erase(it);
		}
	}

}

bool Path_Manager::Get_Path_Listen_Sock(Path_Key in_key, Path_Key_Accept_Sock & out_accept_sock)
{
	{
		AutoLock_Mutex auto_lock(&_accept_dict_lock);

		//1.find key
		std::map<Path_Key,Path_Key_Accept_Sock,Path_Comp>::iterator it = _accept_dict.find(in_key);
		if ( it != _accept_dict.end())
		{
			out_accept_sock = it->second;
		}
		else
		{
			int listen_sd = InitListenSocket((char *)"0.0.0.0",(short) in_key._port); 
			pthread_mutex_t * p_lock = new pthread_mutex_t();
			pthread_mutex_init(p_lock,NULL);
			out_accept_sock._sd = listen_sd;
			out_accept_sock._sd_lock = p_lock;
		}
	}

}

void Path_Manager::Delete_Path_Listen_Sock(Path_Key in_key)
{
	{
		AutoLock_Mutex auto_lock(&_accept_dict_lock);

		//1.find key
		std::map<Path_Key,Path_Key_Accept_Sock,Path_Comp>::iterator it = _accept_dict.find(in_key);
		if ( it != _accept_dict.end())
		{
			if (it->second._sd > 0 )
			{
				close(it->second._sd);
			}

			if (it->second._sd_lock)
			{
				pthread_mutex_destroy(it->second._sd_lock);
				delete it->second._sd_lock;
			}
			_accept_dict.erase(it);
		}
	}

}


