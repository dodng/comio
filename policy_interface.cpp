#include "policy_interface.h"
#include "easy_log.h"
#include "common.h"
#include "io_dispatch.h"

////////////////////////////////
//notice multithread is safe ? yes,add mutex lock
Path_Manager g_path_m;

Json::Reader g_json_reader;
pthread_mutex_t g_json_reader_lock = PTHREAD_MUTEX_INITIALIZER;

Json::FastWriter g_json_writer;
pthread_mutex_t g_json_writer_lock = PTHREAD_MUTEX_INITIALIZER;

//Search_Engine g_search_engine(DUMP_FILE_PATH,LOAD_FILE_PATH);
//log
extern easy_log g_log;

////////////////////////////////

inline std::string int_to_string(int value)
{
	std::string ret_str;
	char value_arr[32] = {0};
	snprintf(value_arr,sizeof(value_arr),"%d",value);
	ret_str = value_arr;
	return ret_str;
}
#if 0
inline std::string float_to_string(float value)
{
	std::string ret_str;
	char value_arr[32] = {0};
	snprintf(value_arr,sizeof(value_arr),"%.4f",value);
	ret_str = value_arr;
	return ret_str;
}
#endif
inline void cook_send_buff(std::string & str,char *buff_p,int buff_len,int &cooked_len)
{
	if (str.size() <= 0
		|| 0 == buff_p
		|| buff_len <= 0
		|| cooked_len < 0)
	{return;}

	if ( (cooked_len + str.size())> buff_len) {return;}

	memcpy(buff_p + cooked_len,str.c_str(),str.size());
	cooked_len += str.size();
}

inline void vector_to_json(std::vector<std::string> &out_vec,std::string key_name,Json::Value &out_json)
{
	//cook return data
	std::string ret_array_str;
	Json::Value ret_json;

	for (int i = 0;i < out_vec.size();i++)
	{
		if (i == 0) {ret_array_str += "[";}

		if (i == (out_vec.size() - 1))
		{ ret_array_str += out_vec[i] + "]"; }
		else
		{ ret_array_str += out_vec[i] + ","; }
	}

	{//enter auto mutex lock
		AutoLock_Mutex auto_lock0(&g_json_reader_lock);
		if (g_json_reader.parse(ret_array_str, ret_json))
		{
		}
	}

	out_json[key_name] = ret_json;
}

inline void vector_to_json(std::vector<int> &out_vec,std::string key_name,Json::Value &out_json)
{
	//cook return data
	std::string ret_array_str;
	Json::Value ret_json;

	for (int i = 0;i < out_vec.size();i++)
	{
		if (i == 0) {ret_array_str += "[";}

		if (i == (out_vec.size() - 1))
		{ ret_array_str += int_to_string(out_vec[i] ) + "]"; }
		else
		{ ret_array_str += int_to_string(out_vec[i] ) + ","; }
	}

	{//enter auto mutex lock
		AutoLock_Mutex auto_lock0(&g_json_reader_lock);
		if (g_json_reader.parse(ret_array_str, ret_json))
		{
		}
	}

	out_json[key_name] = ret_json;

}
////////////////////////////////

//true return 0,other return > 0
int policy_entity::parse_in_json()
{
	if (0 == it_http) {return 1;}
	if (!it_http->parse_over())	{return 2;}

	std::map<std::string,std::string>::iterator it;
	//find data field
	it = it_http->body_map.find("data");
	if (it == it_http->body_map.end()) {return 3;}

	std::string data_str = url_decode(it->second);
	{//enter mutex auto lock
		AutoLock_Mutex auto_lock0(&g_json_reader_lock);
		//parse use json
		if (g_json_reader.parse(data_str, json_in)) 
		{
			return 0;
		}
	}
	return 5;
}

//true return 0,other return > 0
int policy_entity::get_out_json()
{
	int ret = -1;
	//check must field if exist
	if ( json_in["in_path"].isNull()
			|| json_in["out_path"].isNull() 
			|| json_in["open_thread_num"].isNull() 
			|| json_in["close_thread_num"].isNull() )
	{
		ret = 100;
		json_out["ret_code"] = ret;
		return ret;
	}

	int in_path = json_in["in_path"].asInt();
	int open_thread_num = json_in["open_thread_num"].asInt();
	int close_thread_num = json_in["close_thread_num"].asInt();

	//check values
	if (in_path <= 0 
			|| open_thread_num < 0
			|| close_thread_num < 0
	   )
	{
		ret = 101;
		json_out["ret_code"] = ret;
		return ret;
	}

	if (!json_in["out_path"].isArray()
			|| json_in["out_path"].size() <= 0)
	{
		ret = 102;
		json_out["ret_code"] = ret;
		return ret;
	}

	//cook values
	Path_Key key(in_path);
	std::vector<Path_Value> value_vec;
	for (unsigned int i = 0; i < json_in["out_path"].size(); i++)
	{
		//check one value
		std::string ip = json_in["out_path"][i]["ip"].asString();
		int port = json_in["out_path"][i]["port"].asInt();
		int ratio = json_in["out_path"][i]["ratio"].asInt();
		if ( ip.size() > 7 && port > 0 && ratio >= 0)
		{
			Path_Value one_value(ip, port, ratio);
			value_vec.push_back(one_value);
		}
		else
		{
		}
	}

	//check out_path
	if (value_vec.size() <= 0)
	{
		ret = 200;
		json_out["ret_code"] = ret;
		return ret;
	}

		
	//update path_manager
	g_path_m.Update_Path(key,value_vec);

	//lanuch thread
	for (int i = 0 ;i < open_thread_num; i++)
	{
		int lanuch_ret = io_launch_one_thread(key, g_path_m);
	}

	ret = 0;
	json_out["ret_code"] = ret;
	return ret;
}

//true return 0,other return > 0
int policy_entity::cook_senddata(char *send_buff_p,int buff_len,int &send_len)
{
	if (0 == it_http || 0 == send_buff_p) {return 1;}
	if (buff_len <= 0 || send_len < 0) {return 3;}

	std::string json_str;
	{//enter mutex auto lock
		AutoLock_Mutex auto_lock0(&g_json_writer_lock);
		json_str = g_json_writer.write(json_out);
	}
	if (json_str.size() <= 3) {return 2;}//null is 3 length
	if (json_str.size() >= buff_len) {return 3;}//json_str is too long

	std::string str = "HTTP/1.1 200 OK\r\nServer: comio\r\n";
	cook_send_buff(str,send_buff_p,buff_len,send_len);
	str = "Content-Length: " + int_to_string(json_str.size()) + "\r\n\r\n";
	cook_send_buff(str,send_buff_p,buff_len,send_len);
	cook_send_buff(json_str,send_buff_p,buff_len,send_len);
	return 0;

}

#if 0
std::string policy_entity::print_all()
{
	std::string rst;
	rst += "json in\n";
	rst += json_in.toStyledString();
	rst += "json out\n";
	rst += json_out.toStyledString();
	return rst;

}
#endif

int policy_entity::do_one_action(http_entity *it_http_p,char *send_buff_p,int buff_len,int &send_len)
{
	int ret1 = 0,ret2 = 0,ret3 = 0;
	reset();
	set_http(it_http_p);
	ret1 = parse_in_json();
	ret2 = get_out_json();
	ret3 = cook_senddata(send_buff_p,buff_len,send_len);
	//if error occur
	if (ret3 != 0)
	{
		std::string str = "HTTP/1.1 200 OK\r\nServer: comse\r\n";
		std::string body_str = "parse_in_json: " + int_to_string(ret1) + "&";
		body_str += "get_out_json: " + int_to_string(ret2) + "&"; 
		body_str += "cook_senddata: " + int_to_string(ret3);
		//jisuan size
		str += "Content-Length: " + int_to_string(body_str.size()) + "\r\n\r\n";

		cook_send_buff(str,send_buff_p,buff_len,send_len);
		cook_send_buff(body_str,send_buff_p,buff_len,send_len);
	}
	return ret3;
}

bool policy_interface_init_once()
{
	bool se_ret = true;
	return se_ret;
}
