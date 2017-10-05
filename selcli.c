/*
* File : selcli.c
* Created on 2017.9.27
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>


#define ERR_EXIT(m) \
	do { \
		perror(m); \
		exit(EXIT_FAILURE); \
	}while(0)


void str_cli(int sockfd)
{
	fd_set rset;
	struct timeval tv;
	FD_ZERO(&rset);
	int retval;
	int maxfd = sockfd + 1;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	char sendbuf[1024];

	while(1)
	{
		FD_SET(0, &rset);
		FD_SET(sockfd, &rset);
		retval = select(maxfd, &rset, NULL, NULL, &tv);
		if(retval == -1)
			ERR_EXIT("select error");
		if(retval == 0)
			continue;

		if(FD_ISSET(sockfd, &rset))
		{
			int ret = read(sockfd, sendbuf, 1024);
			if(ret == -1)
				ERR_EXIT("read error");
			else if(ret == 0)
			{
				printf("server close\n");
				break;
			}
			printf("%s\n", sendbuf);
			bzero(&sendbuf, 1024);
		}

		if(FD_ISSET(0, &rset))
		{
			if(scanf("%s", sendbuf) == EOF)
				break;
			write(sockfd, sendbuf, 1024);
			bzero(&sendbuf, 1024);
		}
	}
	
	close(sockfd);
}

int main(int argc, char** argv)
{
	int clientFd[1];
	struct sockaddr_in serveraddr;
	
	for(int i = 0; i < 1; i++)
	{
		clientFd[i] = socket(AF_INET, SOCK_STREAM, 0);   //创建socket
		if(clientFd[i] < 0)
			ERR_EXIT("socket error");

		bzero(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(1234);
		inet_pton(AF_INET, "0.0.0.0", &serveraddr.sin_addr);

		if(connect(clientFd[i], (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0)  //连接到服务器
		{
			close(clientFd[i]);
			ERR_EXIT("connect error");
		}
	}
	
	str_cli(clientFd[0]);
	
	exit(0);
}