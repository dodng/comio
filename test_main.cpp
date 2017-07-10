#include <stdio.h>
#include <unistd.h>

#define TEST_3

#ifdef TEST_1
#include "path_manager.h"



int main()
{

	Path_Key key(8888);
	std::vector<Path_Value> in_value_vec;
	Path_Value tmp2("127.0.0.1",9999,5);
	Path_Value tmp3("127.0.0.1",9999,4);
	Path_Value tmp5("127.0.0.1",9999,1);
	in_value_vec.push_back(tmp2);
	in_value_vec.push_back(tmp3);
	in_value_vec.push_back(tmp5);
	
	Path_Manager path_m;
	
	path_m.Update_Path(key,in_value_vec);

	for (int i = 0;i< 1000;i++)
	{
		Path_Value res;
		bool ret = path_m.Select_Path(key,res);
		printf("ret=%d ip:%s\tport:%d\tratio:%d\n",ret,res._ip.c_str(),res._port,res._ratio_config);
	}	
}
#endif

#ifdef TEST_2
#include "io_manager.h"
#include <string>

int main()
{
	Io_Manager _io_m;
	std::string data = "hello";

	for (int i = 0 ;i < 3;i++)
	{	
		int sd = _io_m.get_io("127.0.0.1",8888);
		if (sd > 0)
		{
			int ret = send(sd,data.c_str(),data.size(),0);
			printf("sd:%d ret:%d\n",sd,ret);
		}
		else
		{
			printf("sd:%d\n",sd);
		}
//		_io_m.close_io("127.0.0.1",8888);
		printf("sleep\n");
		sleep(3);
	}

}
#endif


#ifdef TEST_3

#include "io_dispatch.h"

int main()
{
	io_dispatch_main();
}
#endif
