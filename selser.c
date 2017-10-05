/*
* File : selser.c
* Created on 2017.9.27
*/

#include <stdio.h>
#include <stdlib.h>        //exit(-1)
#include <string.h>        //提供比如 bzero,bcopy,bcmp,memset,memcpy memcmp 等函数
#include <sys/socket.h>    //提供socket函数及数据结构
#include <netinet/in.h>    //定义数据结构sockaddr_in
#include <arpa/inet.h>     //提供IP地址转换函数如inet_pton，inet_ntop
#include <errno.h>         //报错函数
#include <signal.h>        //信号函数
#include <unistd.h>        //close函数
#include <sys/wait.h>      //wait函数
#include <sys/types.h>
#include <sys/time.h>


#define ERR_EXIT(m) \
	do { \
		perror(m); \
		exit(EXIT_FAILURE); \
	}while(0)


int main(int argc, char** argv)
{
	signal(SIGPIPE, SIG_IGN);
	
	int listenfd;  //被动套接字(文件描述符），即只可以accept, 监听套接字
	if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		ERR_EXIT("socket error");

	struct sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(1234);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); */
    /* inet_aton("127.0.0.1", &servaddr.sin_addr); */

	int on = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)  //不必等待TIME_WAIT状态就可以重启服务器
		ERR_EXIT("socket error");

	if(bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
		ERR_EXIT("bind error");

	if(listen(listenfd, SOMAXCONN) < 0)  // /proc/sys/net/core/somaxconn 定义系统中每一个端口最大的监听队列的长度
		ERR_EXIT("listen error");
	
	struct sockaddr_in clientaddr;
	socklen_t connlen = sizeof(clientaddr);
	bzero(&clientaddr, connlen);

	int connfd;
	int maxi;
	int clients[FD_SETSIZE];
	for(int i = 0; i < FD_SETSIZE; i++)
		clients[i] = -1;

	//下面是select用到的变量定义
	int nready;
	int i;
	int maxfd = listenfd;
	fd_set rset;
	fd_set allset;
	FD_ZERO(&rset);
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	while(1)
	{
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if(nready == -1)
		{
			if(errno == EINTR)
				continue;
			ERR_EXIT("select error");
		}

		if(nready == 0)
			continue;

		if(FD_ISSET(listenfd, &rset))
		{
			printf("listenfd is in rset\n");

			connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &connlen);
			if(connfd < 0)
			{
				if(errno == EINTR)
					continue;
				else
					ERR_EXIT("accept error");
			}

			for(i = 0; i < FD_SETSIZE; i++)
			{
				if(clients[i] < 0)
				{
					clients[i] = connfd;
					if(i > maxi)
						maxi = i;
					break;
				}
			}

			if(i == FD_SETSIZE)
				ERR_EXIT("too many clients");

			printf("recv connect ip = %s port = %d\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port));

			FD_SET(connfd, &allset);
			if(connfd > maxfd)
				maxfd = connfd;

			if(--nready <= 0)
				continue;
		}

		for(i = 0; i < FD_SETSIZE; i++)
		{
			connfd = clients[i];
			if(connfd == -1)
				continue;

			if(FD_ISSET(connfd, &rset))
			{
				char recvbuf[1024] = {0};
				int ret = read(connfd, recvbuf, 1024);
				if(ret == -1)
					ERR_EXIT("read error");
				else if(ret == 0)
				{
					printf("client close\n");
					FD_CLR(connfd, &allset);
					clients[i] = -1;
					close(connfd);
				}
				write(connfd, recvbuf, 1024);
			}
		}
	}
}