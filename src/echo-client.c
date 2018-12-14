#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>


#define ECHO_PORT		31700
#define MAX_EVENT_NB	32
#define BUF_SIZE		512

static int loop = 1;

static void
__sig_handler(int signo)
{
	if (signo == SIGINT) {
		loop = 0;
		fprintf(stdout, "Recv SIGINT\n");
	}
}

static int
__set_nonblocking(int sock)
{
	int opts;

	opts=fcntl(sock, F_GETFL);

	if (opts < 0) {
		perror("fcntl(sock,GETFL)");
		return -1;
	}

	opts = opts | O_NONBLOCK;

	if(fcntl(sock, F_SETFL, opts) < 0) {  
		perror("fcntl(sock,SETFL,opts)");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	struct sockaddr_in server_addr;
	int connfd, epollfd;
	struct epoll_event ev, events[MAX_EVENT_NB];
	char buf[BUF_SIZE] = {0};

	if (argc < 2) {
		fprintf(stderr, "Usage: ./echo-client <server-ip>\n");
		return 1;
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(ECHO_PORT);
	ret = inet_aton(argv[1], &server_addr.sin_addr);
	if (!ret) {
		fprintf(stderr, "Invalid server address: %s\n", argv[1]);
		return 1;
	}

	connfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connfd == -1) {
		fprintf(stderr, "Failed to exec socket(...)\n");
		return 1;
	}

	ret = connect(connfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		fprintf(stderr, "Failed to exec connect(...)\n");
		goto close_server;
	}

	ret = __set_nonblocking(connfd);
	if (ret < 0) {
		fprintf(stderr, "Failed to set server socket to non-blocking mode\n");
		goto close_server;
	}

	epollfd = epoll_create(MAX_EVENT_NB);
	if (epollfd == -1) {
		fprintf(stderr, "Failed to exec epoll_create(...)\n");
		ret = -1;
		goto close_server;
	}

	ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
	ev.data.fd = connfd;

	epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);

//	signal(SIGINT, __sig_handler);

	while (loop) {
		int i, nfd = 0, nread = 0, nsend = 0;

		nfd = epoll_wait(epollfd, events, MAX_EVENT_NB, 100);

		for (i = 0; i < nfd; i++) {
			if (events[i].events & EPOLLIN) {
				fprintf(stdout, "EPOLLIN\n");

				memset(buf, 0, BUF_SIZE);
				nsend = 0;

				while (1) {
					nread = read(events[i].data.fd, buf + nsend, BUF_SIZE - nsend - 1);
					if (nread == -1 && errno != EAGAIN) {
						fprintf(stderr, "Failed to read data\n");
						ret = -1;
						goto close_epollfd;
					}
					else if (nread == -1 && errno == EAGAIN) {
						break;
					}
					nsend += nread;
				}
				fprintf(stdout, "%s", buf);
			}

			if (events[i].events & EPOLLOUT) {
				fprintf(stdout, "EPOLLOUT\n");

				memset(buf, 0, BUF_SIZE);
				fgets(buf, BUF_SIZE, stdin);
				nread = strlen(buf);
				nsend = 0;

				while (nread > 0) {
					nsend = write(events[i].data.fd, buf + nsend, nread);
					if (nsend < 0 && errno != EAGAIN) {
						fprintf(stderr, "Failed to send data\n");
						ret = -1;
						goto close_epollfd;
					}
					nread -= nsend;
				}
			}

		}
	}

	close(connfd);
	close(epollfd);
	return 0;

close_epollfd:
	close(epollfd);

close_server:
	close(connfd);
	return ret;
}
