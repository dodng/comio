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


#endif
