#ifndef _H_COMMON
#define _H_COMMON

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

class AutoLock_Mutex
{
	public:
		AutoLock_Mutex(pthread_mutex_t *p_mutex)
		{
			if (p_mutex == 0 )
				return;
			m_pmutex = p_mutex;
			pthread_mutex_lock(m_pmutex);
		}
		~AutoLock_Mutex()
		{
			if (m_pmutex == 0)
				return;
			pthread_mutex_unlock(m_pmutex);
		}
	private:
		pthread_mutex_t *m_pmutex;
};

//0:ok,else:failed
int get_sock_status(int fd,int option);
int get_sock_tcp_status(int fd);
int InitListenSocket(char *ip, short port);

inline int my_time_diff(struct timeval &start,struct timeval &end)
{
        if (start.tv_usec <= end.tv_usec)
        {
                return (end.tv_usec-start.tv_usec);
        }
        else
                return (1000000 - start.tv_usec + end.tv_usec);
}

inline void my_time_add(struct timeval & add,struct timeval &n)
{
	if (n.tv_usec + add.tv_usec >= 1000000)
	{
		n.tv_sec += add.tv_sec;
		n.tv_sec += 1;
		n.tv_usec = (n.tv_usec + add.tv_usec) - 1000000;
	}
	else
	{
		n.tv_sec += add.tv_sec;
		n.tv_usec += add.tv_usec;
	}
}

#endif
