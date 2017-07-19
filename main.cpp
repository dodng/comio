#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <event.h>
#include <pthread.h>
#include "evutil.h"
#include "easy_log.h"
#include "main.h"
#include <fstream> 
#include "json/json.h"
#include <string>

//////////////global structure///////////////////////////
/////

pthread_t update_ptocess_thread[UPDATE_PROCESS_THREAD] = {0};

struct sig_ev_arg update_notify_arg[UPDATE_PROCESS_THREAD] = {0};

easy_log g_log("./log/comio.log",(1000000),8);
/////////////////////////////////////////////////////////

int g_recv_buff = (1024 * 4);
int g_send_buff = (1024 * 4);

short g_update_port = 9999;
char g_update_ip[64] = "0.0.0.0";
int g_listen_backlog = -1;

//timeout
struct timeval tv_con_timeout = {0};
struct timeval tv_recv_timeout = {0,100000};
struct timeval tv_send_timeout = {0,0};//not use,send event occur very fast.Use it occur many send timeout.


/////////////////////////////////////////////////////////////

struct interface_data * it_pool_get()
{
	return new interface_data(g_recv_buff,g_send_buff);
}

void it_pool_put(struct interface_data *p_it)
{
	if (p_it) {delete p_it;}
}

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
//////////////callback function/////////////////
void SendData(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct interface_data *p_it = (struct interface_data *)arg;
	struct event *p_ev = &(p_it->ev);
	struct event_base *base = p_ev->ev_base;
	char *buff = (char *)(p_it->send_buff);
	int buff_len = p_it->send_buff_len;
	int len;
	char log_buff[BUFF_SIZE] = {0};

	if (events == EV_TIMEOUT)
	{
		close(p_ev->ev_fd);
		event_del(p_ev);
		it_pool_put(p_it);
		snprintf(log_buff,sizeof(log_buff),"send timeout fd=[%d] error[%d]", fd, errno);
		g_log.write_record(log_buff);
		return;
	} 

	len = send(fd, buff, p_it->now_send_len, MSG_NOSIGNAL);
	if (p_it->now_send_len < p_it->send_buff_len) {p_it->send_buff[p_it->now_send_len] = '\0';}

	gettimeofday(&p_it->l_time[5],0);
	event_del(p_ev);

	if(len >= 0)
	{
		snprintf(log_buff,sizeof(log_buff),
				"Thread[%x]\tClient[%d]"
				"All_time[accept2recv_event=%d,recv=%d,parse_recv=%d,do_policy=%d,send=%d]"
				":Send=%s",
				(int)pthread_self(), fd, 
				my_time_diff(p_it->l_time[0], p_it->l_time[1]), 
				my_time_diff(p_it->l_time[1], p_it->l_time[2]), 
				my_time_diff(p_it->l_time[2], p_it->l_time[3]), 
				my_time_diff(p_it->l_time[3], p_it->l_time[4]), 
				my_time_diff(p_it->l_time[4], p_it->l_time[5]),
				buff);
		g_log.write_record(log_buff);
		//send success
		p_it->status = status_send;
		//reset
		p_it->reset();
		//regist recv event
		reg_add_event(p_ev,p_it->sd,EV_READ,RecvData,p_it,base,&tv_recv_timeout);
	}
	else
	{
		snprintf(log_buff,sizeof(log_buff),"send over[fd=%d] error[%d]:%s", fd, errno, strerror(errno));
		g_log.write_record(log_buff);
		goto GC;
	}
	return;
GC:
	close(p_ev->ev_fd);
	it_pool_put(p_it);
	return;
}

void RecvData(int fd, short int events, void *arg)
{
	if ( 0 == arg || fd <= 0)
		return;
	struct interface_data *p_it = (struct interface_data *)arg;
	struct event *p_ev = &(p_it->ev);
	struct event_base *base = p_ev->ev_base;
	char *buff = (char *)(p_it->recv_buff) + p_it->now_recv_len;
	int buff_len = p_it->recv_buff_len - p_it->now_recv_len;
	int len;
	char log_buff[BUFF_SIZE] = {0};

	//timeout
	if (events == EV_TIMEOUT)
	{
		close(p_ev->ev_fd);
		event_del(p_ev);
		it_pool_put(p_it);
		snprintf(log_buff,sizeof(log_buff),"recv timeout fd=[%d] error[%d]", fd, errno);
		g_log.write_record(log_buff);
		return;
	} 

	// receive data
	gettimeofday(&p_it->l_time[1],0);
	len = recv(fd,buff, buff_len, 0);
	gettimeofday(&p_it->l_time[2],0);
	event_del(p_ev);

	if(len > 0)
	{
		//read success
		p_it->status = status_recv;
		//regist send event
		p_it->now_recv_len += len;
		if (p_it->now_recv_len < p_it->recv_buff_len) {p_it->recv_buff[p_it->now_recv_len] = '\0';}
		int parse_ret = p_it->http.parse_done((char *)(p_it->recv_buff));

		snprintf(log_buff,sizeof(log_buff),"Thread[%x]\tClient[%d]:Recv=[%s]:Parse=[%d]",(int)pthread_self(),fd, buff, parse_ret);
		g_log.write_record(log_buff);

		if (parse_ret < 0 )
		{goto GC;}
		else if (parse_ret == 0)	
		{reg_add_event(p_ev,p_it->sd,EV_READ,RecvData,p_it,base,&tv_recv_timeout);}
		else
		{
			gettimeofday(&p_it->l_time[3],0);
			p_it->policy.do_one_action(&p_it->http,p_it->send_buff,p_it->send_buff_len,p_it->now_send_len);
			gettimeofday(&p_it->l_time[4],0);
			SendData(p_it->sd,EV_WRITE,p_it);
			//			reg_add_event(p_ev,p_it->sd,EV_WRITE,SendData,p_it,base,NULL);
		}

	}
	else
	{
		if (len == 0)
		{
			snprintf(log_buff,sizeof(log_buff),"[fd=%d] , closed gracefully.", fd);
			g_log.write_record(log_buff);
		}
		else
		{	
			snprintf(log_buff,sizeof(log_buff),"recv[fd=%d] error[%d]:%s", fd, errno, strerror(errno));
			g_log.write_record(log_buff);
		}
		goto GC;
	}
	return;

GC:
	close(p_ev->ev_fd);
	it_pool_put(p_it);
	return;
}

void UpdateAcceptListen(int fd, short what, void *arg)
{
	struct event *p_ev = (struct event *)arg;
	char log_buff[128] = {0};
	struct sockaddr_in sin;
	socklen_t len = sizeof(struct sockaddr_in);
	int accept_fd = 0;

	//one thread ,not need lock
	accept_fd = accept(fd, (struct sockaddr*)&sin, &len);

	if (accept_fd > 0)
	{
		int iret = 0;
		if((iret = fcntl(accept_fd, F_SETFL, O_NONBLOCK)) < 0)
		{   
			snprintf(log_buff,sizeof(log_buff),"%s fd=%d: fcntl nonblocking error:%d", __func__, accept_fd, iret);
			g_log.write_record(log_buff);
			return;
		}  
		// add a read event for receive data
		struct interface_data *p_read_it = it_pool_get();
		p_read_it->sd = accept_fd;
		gettimeofday(&p_read_it->l_time[0],0);
		reg_add_event(&(p_read_it->ev),accept_fd,EV_READ,RecvData,p_read_it,p_ev->ev_base,&tv_recv_timeout);
		snprintf(log_buff,sizeof(log_buff),"process thread read new client fd[%d]",accept_fd);
		g_log.write_record(log_buff);

	}
	else 
	{
		snprintf(log_buff,sizeof(log_buff),"%s[%d]\t%s:accept  error return:%d, errno:%d", __FILE__,__LINE__,__func__,accept_fd,errno);
		g_log.write_record(log_buff);
	}

}

////////////////////////////////////////////

///////////process thread/////////////////

static void *UpdateProcessThread(void *arg)
{
	if (0 == arg) pthread_exit(NULL);
	struct sig_ev_arg *p_ev_arg = (struct sig_ev_arg *)arg;
	struct event notify_event;
	struct event_base *base = event_init();
	reg_add_event(&notify_event,p_ev_arg->listen_fd,EV_READ | EV_PERSIST, UpdateAcceptListen, &notify_event,base,NULL);
	event_base_dispatch(base);
	pthread_exit(NULL);
}

////////////////////////////////////////////

//succ return 0,other return < 0

int parse_config(char *config_path)
{
	int ret = 0;
	if (0 == config_path) {return -1;}
	Json::Reader reader;  
	Json::Value root;
	std::ifstream is;
	is.open(config_path);

	if (!is.is_open())
	{
		ret = -2;
	}

	if (reader.parse(is, root))  
	{
		//parse
	}
	else
	{
		ret = -3;
	}

	is.close();
	return ret;	
}

int main (int argc, char **argv)
{
	char log_buff[128] = {0};

	//policy_interface_init_once
	int ret = policy_interface_init_once();
	snprintf(log_buff,sizeof(log_buff),"policy_interface_init_once return:%d",ret);
	g_log.write_record(log_buff);

	//parse config
	if (argc == 2)
	{
		int ret = parse_config(argv[1]);
		snprintf(log_buff,sizeof(log_buff),"parse_config[%s] return:%d",argv[1],ret);
		g_log.write_record(log_buff);

	}
	// init search and update port
	int update_port_sock = InitListenSocket(g_update_ip, g_update_port);
	//check sock
	if (update_port_sock <= 0)
	{
		snprintf(log_buff,sizeof(log_buff),"update[%d] sock listen error[%d]", update_port_sock, errno);
		g_log.write_record(log_buff);
		return -1;
	}

	//create update threads
	for(int i = 0 ; i < UPDATE_PROCESS_THREAD;i++)
	{
		update_notify_arg[i].listen_fd = update_port_sock;
		int err = pthread_create(&update_ptocess_thread[i],0,UpdateProcessThread,&update_notify_arg[i]);
		if (err != 0)
		{
			snprintf(log_buff,sizeof(log_buff),"can't create Update ProcessThread thread:%s",strerror(err));
			g_log.write_record(log_buff);
			return -1;
		}
	}

	for (int i = 0 ; i < UPDATE_PROCESS_THREAD ;i++)
	{
		pthread_join(update_ptocess_thread[i],0);
	}
	return (0);
}

