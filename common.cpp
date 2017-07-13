#include "common.h"

int get_sock_status(int fd,int option)
{
	int error;
	socklen_t len;

	len=sizeof(error);
	getsockopt(fd, SOL_SOCKET, option, &error, &len);
	return error;

}

int get_sock_tcp_status(int fd)
{
	struct tcp_info info; 
	int len=sizeof(info); 
	getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len); 

	if( info.tcpi_state == TCP_ESTABLISHED) 
	{return 0;}
	else
	{return -1;}
}

int InitListenSocket(char *ip, short port) 
{
	if (0 == ip) return 0;

	int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
	int ret = 0;

	fcntl(listenFd, F_SETFL, O_NONBLOCK); // set non-blocking 
	// bind & listen 
	sockaddr_in sin; 
	bzero(&sin, sizeof(sin)); 
	sin.sin_family = AF_INET; 
	sin.sin_addr.s_addr = inet_addr(ip);
	sin.sin_port = htons(port); 
	bind(listenFd, (const sockaddr*)&sin, sizeof(sin)); 
	ret = listen(listenFd, -1); 
	return listenFd;
}


