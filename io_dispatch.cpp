#include "io_dispatch.h"

short g_search_port = 8807;
char g_search_ip[64] = "0.0.0.0";
int g_recv_buff_io = (16384);
struct timeval tv_recv_timeout_io = {0,900000};
struct timeval tv_timer = {0,500000};

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


struct io_dispatch_info * get_io_dispatch_info()
{
	char log_buff[1024] = {0};
	io_dispatch_info * p_tmp = new io_dispatch_info(g_recv_buff_io, g_recv_buff_io);
	gettimeofday(&p_tmp->l_time[0], 0);
	gettimeofday(&p_tmp->l_time[1], 0);
	snprintf(log_buff,sizeof(log_buff),"[%p] get_io_dispatch_info time(%d)"
			,p_tmp,my_time_diff(p_tmp->l_time[0], p_tmp->l_time[1]));
	g_log.write_record(log_buff);

	return p_tmp;
}

void put_io_dispatch_info(struct io_dispatch_info *p_it)
{
	char log_buff[1024] = {0};
	snprintf(log_buff,sizeof(log_buff),"[%p] put_io_dispatch_info", p_it);
	g_log.write_record(log_buff);
	if (p_it) 
	{
		delete p_it;
	}
}

void ProcessTimer(int fd, short int events, void *arg)
{
	if ( 0 == arg)
		return;
	struct io_dispatch_thread_info *p_it = (struct io_dispatch_thread_info *)arg;
	struct event *p_ev = &(p_it->timer_ev);
	struct event_base *base = p_it->p_event_base;
	event_del(p_ev);

	if ( p_it->thread_status == thread_closing)
	{
		struct timeval closing_wait_time = {0,0};
		my_time_add(tv_recv_timeout_io,closing_wait_time);
		my_time_add(tv_recv_timeout_io,closing_wait_time);
		p_it->timer_tv = closing_wait_time;

		reg_add_event(p_ev,-1,0,ProcessTimer,p_it,base,&(p_it->timer_tv));
		p_it->thread_status = thread_close;

		g_log.write_record("ProcessTimer [closing] reg_add");
	}
	else if (p_it->thread_status == thread_close)
	{
		event_base_free(base);
		//release accept socket
		p_it->p_path_mgr->Put_Path_Listen_Sock(p_it->path_k);

		if (p_it) 
		{
			delete p_it;
			p_it = 0;
		}
		g_log.write_record("event_base_free");
	}
	else
	{
		reg_add_event(p_ev,-1,0,ProcessTimer,p_it,base,&(p_it->timer_tv));
		g_log.write_record("ProcessTimer reg_add");
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
		reg_add_event(&(p_it->p_upstream->_ev),p_it->p_upstream->io_sd,EV_READ,ProcessUpStream_Recv,p_it,base,&tv_recv_timeout_io);
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
	put_io_dispatch_info(p_it);
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

		int downstream_sd = 0;
		int send_len = -1;
		//select which downstream path
		if (p_it->p_thread_info && p_it->p_thread_info->p_path_mgr)
		{
			p_it->p_thread_info->p_path_mgr->Select_Path(p_it->p_thread_info->path_k,p_it->path_res);
		}
		//get io_sd from downstreampath
		downstream_sd = p_it->p_thread_info->io_mgr.get_io(p_it->path_res._ip,(int)p_it->path_res._port);
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
		reg_add_event(&(p_it->p_downstream->_ev),p_it->p_downstream->io_sd,EV_READ,ProcessDownStream_Recv,p_it,base,&tv_recv_timeout_io);
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
	put_io_dispatch_info(p_it);
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

	//if thread is closing
	if (p_thread_info->thread_status != thread_use)
	{
		g_log.write_record("return\tProcessListenEvent");
		return;
	}

	if (accept_fd > 0)
	{
		int iret = 0;
		if((iret = fcntl(accept_fd, F_SETFL, O_NONBLOCK)) < 0)
		{   
			return;
		}  

		struct io_dispatch_info *p_read_it = get_io_dispatch_info();

		p_read_it->p_thread_info = p_thread_info;
		// add a read event for receive data
		p_read_it->p_upstream->set_io_sd(accept_fd);

		reg_add_event(&(p_read_it->p_upstream->_ev),p_read_it->p_upstream->io_sd,
				EV_READ,ProcessUpStream_Recv,p_read_it,
				p_thread_info->p_event_base,&tv_recv_timeout_io);
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
	reg_add_event(&(p_thread_info->timer_ev),-1,0,ProcessTimer,p_thread_info,base,&(p_thread_info->timer_tv));
	event_base_dispatch(base);
	g_log.write_record("IoLoopThread exit");
	pthread_exit(0);
}

//ret 0:ok,else failed

int io_launch_one_thread(Path_Key & in_key, Path_Manager & in_path_mgr)
{
	//get io_dispatch_thread_info
	struct io_dispatch_thread_info * io_thread_p = new io_dispatch_thread_info();
	//init values
	io_thread_p->p_event_base = 0;
	io_thread_p->path_k_sd = 0;
	io_thread_p->p_path_k_lock = 0;
	io_thread_p->p_path_mgr = 0;

	//set values
	io_thread_p->p_path_mgr = &in_path_mgr;
	io_thread_p->thread_status = thread_use;
	io_thread_p->path_k = in_key;
	io_thread_p->timer_tv = tv_timer;

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
		if (io_thread_p->p_path_mgr)
		{io_thread_p->p_path_mgr->Put_Path_Listen_Sock(io_thread_p->path_k);}

		if (io_thread_p)
		{delete io_thread_p;}


		return -1;
	}
	else
	{
		in_path_mgr.Add_Path_Ptr(in_key,(void *)io_thread_p);
		return 0;
	}

	//	sleep(23);
	//	io_thread_p->thread_status = thread_closing;
	//pthread_join(io_thread_p->thread_info,0);

}

int io_retrieve_one_thread(Path_Key & in_key, Path_Manager & in_path_mgr)
{
	struct io_dispatch_thread_info * io_thread_p = 0;
	io_thread_p = (struct io_dispatch_thread_info *)in_path_mgr.Del_Path_Ptr(in_key);

	if (io_thread_p != 0)
	{
		io_thread_p->thread_status = thread_closing;
		return 0;
	}
	else
	{
		return -1;
	}
}
