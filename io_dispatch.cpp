#include "io_dispatch.h"

short g_search_port = 8807;
char g_search_ip[64] = "0.0.0.0";
int g_recv_buff = (16384);
struct timeval tv_recv_timeout = {0,900000};

//log
#if 1
#include "easy_log.h"
#include <errno.h>

extern easy_log g_log;
#define BUFF_SIZE (1024 * 4)
#endif

void inline reg_add_event(struct event *p_eve,int fd,int event_type,event_cb_func p_func,void *p_arg,
		struct event_base *p_base,struct timeval *p_time)
{
	if (0 == p_eve
			|| 0 == p_func
			|| 0 == p_base)
		return;
	event_set(p_eve,fd,event_type,p_func,p_arg);
	event_base_set(p_base,p_eve);
	// add recv timeout
	event_add(p_eve,p_time);
}


struct io_dispatch_info * it_pool_get()
{
	char log_buff[1024] = {0};
	io_dispatch_info * p_tmp = new io_dispatch_info(g_recv_buff, g_recv_buff);
	gettimeofday(&p_tmp->l_time[0], 0);
	gettimeofday(&p_tmp->l_time[1], 0);
	snprintf(log_buff,sizeof(log_buff),"[%p] it_pool_get time(%d)"
			,p_tmp,my_time_diff(p_tmp->l_time[0], p_tmp->l_time[1]));
	g_log.write_record(log_buff);
	/*
	   Io_Storage * p_stream_1 = new Io_Storage(g_recv_buff);
	   Io_Storage * p_stream_2 = new Io_Storage(g_recv_buff);

	   if (p_tmp && p_stream_1 && p_stream_2)
	   {
	   p_tmp->p_upstream = p_stream_1;
	   p_tmp->p_downstream = p_stream_2;
	   gettimeofday(&p_tmp->l_time[1], 0);
	   snprintf(log_buff,sizeof(log_buff),"[%p] it_pool_get time(%d)"
	   ,p_tmp,my_time_diff(p_tmp->l_time[0], p_tmp->l_time[1]));
	   g_log.write_record(log_buff);
	   }
	   else
	   {
	   p_tmp = 0;
	   }*/

	return p_tmp;
}

void it_pool_put(struct io_dispatch_info *p_it)
{
	char log_buff[1024] = {0};
	snprintf(log_buff,sizeof(log_buff),"[%p] it_pool_put", p_it);
	g_log.write_record(log_buff);
	if (p_it) 
	{
		/*
		   if (p_it->p_upstream) 
		   {
		   delete p_it->p_upstream;
		   p_it->p_upstream = 0;
		   }

		   if (p_it->p_downstream) 
		   {
		   delete p_it->p_downstream;
		   p_it->p_downstream = 0;
		   }
		   */
		delete p_it;
	}
}

void ProcessDownStream_Recv(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct io_dispatch_info *p_it = (struct io_dispatch_info *)arg;
	struct event *p_ev = &(p_it->p_downstream->_ev);
	struct event_base *base = p_it->p_thread_info->p_event_base;
	int len;
	char log_buff[BUFF_SIZE] = {0};

	//timeout
	if (events == EV_TIMEOUT)
	{
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessDownStream_Recv\tfd[%d]\ttimeout", p_it, fd);
		g_log.write_record(log_buff);
		event_del(p_ev);
		goto GC;	
	} 

	// receive data from upstream
	len = p_it->p_downstream->process_recv_event(fd);
	event_del(p_ev);

	if(len > 0)
	{
		gettimeofday(&p_it->l_time[4], 0);
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessDownStream_Recv\tfd[%d]\trecv[%d]\ttime(%d)", p_it, fd, len,
				my_time_diff(p_it->l_time[3], p_it->l_time[4]));
		g_log.write_record(log_buff);

		int send_len = -1;

		//senddata to downstream
		send_len = p_it->p_downstream->send_2_peer(p_it->p_upstream->io_sd);
		gettimeofday(&p_it->l_time[5], 0);
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessDownStream_Recv\tfd[%d]\tsend[%d]\ttime(%d)\tsum_time(%d)", p_it, fd, send_len,
				my_time_diff(p_it->l_time[4], p_it->l_time[5]),
				my_time_diff(p_it->l_time[0], p_it->l_time[5]));
		g_log.write_record(log_buff);

		//add recv event for upstream
		reg_add_event(&(p_it->p_upstream->_ev),p_it->p_upstream->io_sd,EV_READ,ProcessUpStream_Recv,p_it,base,&tv_recv_timeout);
		//	g_log.write_record("reg_add_event\tProcessUpStream_Recv");


	}
	else
	{
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessDownStream_Recv\tfd[%d]\trecv[%d]\terrno[%d]", p_it, fd, len, errno);
		g_log.write_record(log_buff);

		if (len == 0)
		{
		}
		else
		{	
		}
		goto GC;
	}
	return;

GC:
	close(p_it->p_upstream->io_sd);
	it_pool_put(p_it);
	return;

}

void ProcessUpStream_Recv(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct io_dispatch_info *p_it = (struct io_dispatch_info *)arg;
	struct event *p_ev = &(p_it->p_upstream->_ev);
	struct event_base *base = p_it->p_thread_info->p_event_base;
	int len;
	char log_buff[BUFF_SIZE] = {0};

	//timeout
	if (events == EV_TIMEOUT)
	{
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessUpStream_Recv\tfd[%d]\ttimeout", p_it, fd);
		g_log.write_record(log_buff);

		event_del(p_ev);
		goto GC;	
	} 

	// receive data from upstream
	len = p_it->p_upstream->process_recv_event(fd);
	event_del(p_ev);

	if(len > 0)
	{
		gettimeofday(&p_it->l_time[2], 0);
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessUpStream_Recv\tfd[%d]\trecv[%d]\ttime(%d)", p_it, fd, len,
				my_time_diff(p_it->l_time[1], p_it->l_time[2]));

		g_log.write_record(log_buff);

		Path_Value path_res;
		int downstream_sd = 0;
		int send_len = -1;
		//select which downstream path
		if (p_it->p_thread_info && p_it->p_thread_info->p_path_mgr)
		{
			p_it->p_thread_info->p_path_mgr->Select_Path(p_it->p_thread_info->path_k,path_res);
		}
		//get io_sd from downstreampath
		downstream_sd = p_it->p_thread_info->io_mgr.get_io(path_res._ip,(int)path_res._port);
		//senddata to downstream
		if (downstream_sd > 0)
		{
			p_it->p_downstream->set_io_sd(downstream_sd);
			send_len = p_it->p_upstream->send_2_peer(downstream_sd);
			gettimeofday(&p_it->l_time[3], 0);
			snprintf(log_buff,sizeof(log_buff),"[%p] ProcessUpStream_Recv\tfd[%d]\tsend[%d]\ttime(%d)", p_it, fd, send_len,
					my_time_diff(p_it->l_time[2], p_it->l_time[3]));

			g_log.write_record(log_buff);
		}
		//add recv event for downstream
		reg_add_event(&(p_it->p_downstream->_ev),p_it->p_downstream->io_sd,EV_READ,ProcessDownStream_Recv,p_it,base,&tv_recv_timeout);
		//	g_log.write_record("reg_add_event\tProcessDownStream_Recv");


	}
	else
	{
		gettimeofday(&p_it->l_time[6], 0);
		snprintf(log_buff,sizeof(log_buff),"[%p] ProcessUpStream_Recv\tfd[%d]\trecv[%d]\terrno[%d]\ttime(%d)", p_it, fd, len, errno,
				my_time_diff(p_it->l_time[5], p_it->l_time[6]));
		g_log.write_record(log_buff);
		if (len == 0)
		{
		}
		else
		{	
		}
		goto GC;
	}
	return;

GC:
	close(p_ev->ev_fd);
	it_pool_put(p_it);
	return;

}

void ProcessListenEvent(int fd, short what, void *arg)
{
	struct io_dispatch_thread_info * p_thread_info = (struct io_dispatch_thread_info * )arg;
	char log_buff[128] = {0};
	struct sockaddr_in sin;
	socklen_t len = sizeof(struct sockaddr_in);
	int accept_fd = 0;

	//accept lock
	pthread_mutex_lock(p_thread_info->p_path_k_lock);
	accept_fd = accept(fd, (struct sockaddr*)&sin, &len);
	pthread_mutex_unlock(p_thread_info->p_path_k_lock);

	if (accept_fd > 0)
	{
		int iret = 0;
		if((iret = fcntl(accept_fd, F_SETFL, O_NONBLOCK)) < 0)
		{   
			return;
		}  

		struct io_dispatch_info *p_read_it = it_pool_get();

		p_read_it->p_thread_info = p_thread_info;
		// add a read event for receive data
		p_read_it->p_upstream->set_io_sd(accept_fd);

		reg_add_event(&(p_read_it->p_upstream->_ev),p_read_it->p_upstream->io_sd,
				EV_READ,ProcessUpStream_Recv,p_read_it,
				p_thread_info->p_event_base,&tv_recv_timeout);
		//g_log.write_record("reg_add_event\tProcessUpStream_Recv");


	}
	else 
	{

	}	
}

static void *IoLoopThread(void *arg)
{
	if (0 == arg) pthread_exit(NULL);
	struct io_dispatch_thread_info * p_thread_info = (struct io_dispatch_thread_info * )arg;
	struct event notify_event;
	struct event_base *base = event_init();
	p_thread_info->p_event_base = base;   //dodng:xx  diff  comse


	reg_add_event(&notify_event,p_thread_info->path_k_sd,EV_READ | EV_PERSIST, ProcessListenEvent, p_thread_info,base,NULL);
	//g_log.write_record("reg_add_event\tProcessListenEvent");
	event_base_dispatch(base);
	pthread_exit(NULL);
}


int io_dispatch_main()
{
	//get path_key and Path_Manager
	Path_Key key(9807);
	std::vector<Path_Value> in_value_vec;
	Path_Value tmp2("127.0.0.1",8817,5);
	in_value_vec.push_back(tmp2);

	Path_Manager path_m;

	path_m.Update_Path(key,in_value_vec);

	//get io_dispatch_thread_info
	struct io_dispatch_thread_info * io_thread_p = new io_dispatch_thread_info();
	//init values
	io_thread_p->p_event_base = 0;
	io_thread_p->path_k_sd = 0;
	io_thread_p->p_path_k_lock = 0;
	io_thread_p->p_path_mgr = 0;

	//set values
	io_thread_p->p_path_mgr = &path_m;
	io_thread_p->thread_status = 0;
	io_thread_p->path_k = key;

	if (io_thread_p->p_path_mgr) 
	{
		Path_Key_Accept_Sock values_tmp;
		io_thread_p->p_path_mgr->Get_Path_Listen_Sock(io_thread_p->path_k,values_tmp);
		io_thread_p->path_k_sd = values_tmp._sd;
		io_thread_p->p_path_k_lock = values_tmp._sd_lock;
	}
	//lanuch  one io_thread
	int err = pthread_create(&(io_thread_p->thread_info),0,IoLoopThread,io_thread_p);
	if (err != 0)
	{
	}

	pthread_join(io_thread_p->thread_info,0);


}
