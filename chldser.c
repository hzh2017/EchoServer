/*
* File : chldser.c
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


#define ERR_EXIT(m) \
	do { \
		perror(m); \
		exit(EXIT_FAILURE); \
	}while(0)


void str_echo(int sockfd)
{
	int ret;
	char readbuf[1024];

	again:
		while( (ret = read(sockfd, readbuf, 1024)) > 0)  //读客户端发送的数据
			write(sockfd, readbuf, 1024);  //写回数据
		if(ret == 0)
			printf("客户端关闭连接\n");
		else if(ret < 0 && errno == EINTR)
		{
			printf("read EINTR");
			goto again;
		}
		else if(ret < 0)
			printf("read error:%s\n", strerror(errno));
}

void sig_chld(int signo)
{
	pid_t pid;
	int stat;

	// pid = wait(&stat);
	// while( (pid = wait(&stat)) > 0)  //无法防止wait在正运行的子进程尚未终止时阻塞
	while( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}


int main(int argc, char** argv)
{
	signal(SIGPIPE, SIG_IGN);
	
	int listenfd, connfd, ret;
	const int on = 1;
	pid_t childpid;
	
	struct sockaddr_in serveraddr, clientaddr;
	socklen_t len;
	char ip[40] = {0};
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);  //创建socket
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));  //不必等待TIME_WAIT状态就可以重启服务器
	if(listenfd < 0)
		ERR_EXIT("socket error");

	if(signal(SIGCHLD, sig_chld) == SIG_ERR)
		ERR_EXIT("signal error");

	bzero(&serveraddr, sizeof(serveraddr));
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(1234);
	inet_pton(AF_INET, "0.0.0.0", &serveraddr.sin_addr);  //将c语言字节序转换为网络字节序

	ret = bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));   //绑定IP和端口
	if(ret < 0)
		ERR_EXIT("bind error");

	ret = listen(listenfd, 5);   //监听
	if(ret != 0)
	{
		close(listenfd);
		ERR_EXIT("listen error");
	}

	bzero(&clientaddr, sizeof(clientaddr));
	len = sizeof(clientaddr);
	
	while(1)
	{
		connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len);  //接受客户端的连接
		printf("%s 连接到服务器\n", inet_ntop(AF_INET, &clientaddr.sin_addr, ip, sizeof(ip)));
		if(connfd < 0)
		{
			if(errno == EINTR)
				continue;
			else
				printf("accept error:%s\n", strerror(errno));
		}

		if( (childpid = fork()) == 0 )
		{
			close(listenfd);
			str_echo(connfd);
			exit(0);
		}
		close(connfd);
	}
}